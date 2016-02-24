
#include "equipment_make.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "singleton.h"
#include "equipt_upgrade.h"
#include "eliteCombat.h"
#include "net.h"
#include "statistics.h"
#include "relation.h"
#include "first_seven_goals.h"
#include "qq_invite.h"
#include "spls_timer.h"
#include "loot.h"

const std::string strEquipLink = "<A HREF=\"event:{'id':$G,'cmd':'showEquip','cid':$C}\" TARGET=\"\"><U>$N</U></A>";

extern std::string strBoxGetMsg;
extern std::string strBoxSilverNotEnoughMsg;

extern Database& GetDb();
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
extern std::string strGold;
extern std::string strSilver;
extern std::string strLing;
extern std::string strPrestige;
//升级品质保留等级消耗金币
#ifdef JP_SERVER
static const int iUpgradeEquiptmentKeepLevelGold = 100;
#else
static const int iUpgradeEquiptmentKeepLevelGold = 50;
#endif
extern std::string strEquiptMakeInfo;
void InsertSaveDb(const std::string& sql);

equipment_scroll_mgr::equipment_scroll_mgr()
{
    load();
}

equipment_scroll* equipment_scroll_mgr::getScroll(int sid)
{
    return m_scrolls[sid].get();
}

equipment_scroll* equipment_scroll_mgr::getScrollBySrcId(int srcid)
{
    return m_scrolls2[srcid].get();
}

void equipment_scroll_mgr::load()
{
    Query q(GetDb());
    q.get_result("select s.scroll,s.eid,s.src_eid,t.id from base_equip_scroll as s left join base_treasures as t on s.scroll=t.value where t.type=8 order by s.scroll");
    while (q.fetch_row())
    {
        equipment_scroll* p = new equipment_scroll();
        boost::shared_ptr<equipment_scroll> sp(p);

        p->m_scroll_id = q.getval();
        p->m_make_equipment = q.getval();
        p->m_src_equipment = q.getval();
        p->m_gem_id = q.getval();

        p->m_equipment_src = GeneralDataMgr::getInstance()->GetBaseEquipment(p->m_src_equipment);

        p->m_equipment = GeneralDataMgr::getInstance()->GetBaseEquipment(p->m_make_equipment);
        if (!p->m_equipment || !p->m_equipment_src)
        {
            ERR();
            continue;
        }
        
        p->m_eqobj.push_back( Pair("name", p->m_equipment->name));
        p->m_eqobj.push_back( Pair("spic", p->m_equipment->baseid));
        p->m_eqobj.push_back( Pair("quality", p->m_equipment->quality));
        p->m_eqobj.push_back( Pair("type", p->m_equipment->type));

        m_scrolls[p->m_scroll_id] = sp;
        m_scrolls2[p->m_src_equipment] = sp;
    }
    q.free_result();

    q.get_result("select scroll,tid,num from base_equip_scroll_m where 1 order by scroll");
    while (q.fetch_row())
    {
        int scroll_id = q.getval();
        if (m_scrolls[scroll_id].get())
        {
            int tid = q.getval();
            boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
            if (!bt.get())
            {
                ERR();
            }
            else
            {
                m_scrolls[scroll_id]->m_mlist.push_back(std::make_pair(bt, q.getval()));
                //材料可制作装备
                if (!bt->canMakeInited)
                {
                    bt->canMake.clear();
                    bt->canMakeInited = true;
                }
                if (m_scrolls[scroll_id]->m_equipment.get())
                {
                    json_spirit::Object o;
                    o.push_back( Pair("type", iItem_type_equipment) );
                    o.push_back( Pair("name", m_scrolls[scroll_id]->m_equipment->name) );
                    o.push_back( Pair("quality", m_scrolls[scroll_id]->m_equipment->quality) );
                    o.push_back( Pair("level", m_scrolls[scroll_id]->m_equipment->needLevel) );
                    bt->canMake.push_back(o);
                }
            }
        }
        else
        {
            cout<<"!!!!!! error base_equip_scroll_m, scroll id "<<scroll_id<<endl;
        }
    }
    q.free_result();
}


bool trySubMaterial(equipment_scroll* sp, Bag& m_bag)
{
    for (std::list<std::pair<boost::shared_ptr<baseTreasure>,int> >::iterator it = sp->m_mlist.begin(); it != sp->m_mlist.end(); ++it)
    {
        if (it->first.get())
        {
            int32_t num = m_bag.getGemCount(it->first->id);
            if (num < it->second)
            {
                return false;
            }
        }
    }
    for (std::list<std::pair<boost::shared_ptr<baseTreasure>,int> >::iterator it = sp->m_mlist.begin(); it != sp->m_mlist.end(); ++it)
    {
        if (it->first.get())
        {
            int err_code = 0;
            m_bag.addGem(it->first->id, -it->second, err_code);
        }
    }
    return true;
}


bool checkMaterial(equipment_scroll* sp, CharData& char_data, Bag& m_bag, json_spirit::Array& mlist, int type = 1)
{
    bool canMake = true;
    for (std::list<std::pair<boost::shared_ptr<baseTreasure>,int> >::iterator it = sp->m_mlist.begin(); it != sp->m_mlist.end(); ++it)
    {
        if (it->first.get())
        {
            json_spirit::Object o;
            o.push_back( Pair("name", it->first->name) );
            int32_t num = m_bag.getGemCount(it->first->id);
            if (num >= it->second)
            {
                num = it->second;
            }
            else
            {
                canMake = false;
            }
            o.push_back( Pair("spic", it->first->spic) );
            o.push_back( Pair("type", iItem_type_gem) );
            o.push_back( Pair("id", it->first->id) );
            o.push_back( Pair("quality", it->first->quality) );
            if (it->first->m_place.get())
            {
                bool can_sweep = false;
                o.push_back( Pair("info", it->first->m_place->info) );
                if (it->first->m_place->type == 1)
                {
                    can_sweep = char_data.m_tempo.check_stronghold_can_sweep(it->first->m_place->mapId,it->first->m_place->stageId,it->first->m_place->pos);
                    boost::shared_ptr<StrongholdData> bstronghold = GeneralDataMgr::getInstance()->GetStrongholdData(it->first->m_place->mapId,it->first->m_place->stageId,it->first->m_place->pos);
                    if (bstronghold.get())
                    {
                        o.push_back( Pair("open_level", bstronghold->m_level) );
                    }
                    o.push_back( Pair("can_sweep", can_sweep) );
                }
                else if (it->first->m_place->type == 2)
                {
                    can_sweep = eliteCombatMgr::getInstance()->check_stronghold_can_attack(char_data.m_id,it->first->m_place->mapId,it->first->m_place->pos);
                    o.push_back( Pair("can_sweep", can_sweep) );
                }
            }

            o.push_back( Pair("num", num) );
            o.push_back( Pair("need", it->second) );
            mlist.push_back(o);
        }
    }
    //卷轴信息
    int32_t gem_num = m_bag.getGemCount(sp->m_gem_id);
    if (gem_num <= 0)
    {
        canMake = false;
    }
    if (type == 2)
    {
        boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(sp->m_gem_id);
        if (bt.get() && bt->m_place.get())
        {
            json_spirit::Object o;
            o.push_back( Pair("name", bt->name) );
            o.push_back( Pair("spic", bt->spic) );
            o.push_back( Pair("type", iItem_type_gem) );
            o.push_back( Pair("id", bt->id) );
            o.push_back( Pair("quality", bt->quality) );
            o.push_back( Pair("num", gem_num) );
            o.push_back( Pair("need", 1) );
            o.push_back( Pair("info", bt->m_place->info) );
            bool can_sweep = false;
            if (bt->m_place->type == 1)
            {
                can_sweep = char_data.m_tempo.check_stronghold_can_sweep(bt->m_place->mapId,bt->m_place->stageId,bt->m_place->pos);
                o.push_back( Pair("can_sweep", can_sweep) );
            }
            else if (bt->m_place->type == 2)
            {
                can_sweep = eliteCombatMgr::getInstance()->check_stronghold_can_attack(char_data.m_id,bt->m_place->mapId,bt->m_place->pos);
                o.push_back( Pair("can_sweep", can_sweep) );
            }
            mlist.push_back(o);
        }
    }
    return canMake;
}

bool canUpgradeEquipment(equipment_scroll* sp, Bag& m_bag)
{
    bool canMake = true;
    for (std::list<std::pair<boost::shared_ptr<baseTreasure>,int> >::iterator it = sp->m_mlist.begin(); it != sp->m_mlist.end(); ++it)
    {
        if (it->first.get())
        {
            int32_t num = m_bag.getGemCount(it->first->id);
            if (num >= it->second)
            {

            }
            else
            {
                canMake = false;
                break;
            }
        }
    }
    return canMake;
}

void upgradeValue(int quality, int type, int& clevel, int& addValue, int& addValue2)
{
    int cclevel = clevel;
    for (int level = 1; level <= cclevel; ++level)
    {
        int add2 = 0;
        int add = equipmentUpgrade::getInstance()->getUpgradeValue(quality, type, level, add2);
        addValue += add;
        if (add2 > 0)
        {
            addValue2 += add2;
        }
    }
}

//升级装备
int CharData::upgradeEquipment(int eid, bool cost_gold, json_spirit::Object& robj)
{
    if (cost_gold && gold() < 50)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //cout<<"upgradeEquipment(), eid "<<eid<<endl;
    equipment_scroll* sp = NULL;
    EquipmentData* src_eq = m_bag.getEquipById(eid);
    if (!src_eq)
    {
        src_eq = m_generals.getEquipById(eid);
    }
    if (src_eq)
    {
        //cout<<"find scroll by src_id "<<src_eq->baseid<<endl;
        sp = Singleton<equipment_scroll_mgr>::Instance().getScrollBySrcId(src_eq->baseid);
        if (!sp)
        {
            //cout<<"not find"<<endl;
            return HC_ERROR;
        }
    }
    else
    {
        //cout<<"wrong eid"<<endl;
        return HC_ERROR;
    }
    Bag* pbag = src_eq->getContainer();
    //武将身上,判断等级是否够
    if (pbag != &m_bag && pbag && pbag->gd.get() && pbag->gd->m_level < sp->m_equipment->needLevel)
    {
        return HC_ERROR_NOT_ENOUGH_GENERAL_LEVEL;
    }
    if (m_bag.getGemCount(sp->m_gem_id) <= 0)
    {
        //cout<<"no scroll,gem id: "<<sp->m_gem_id<<endl;
        return HC_ERROR;
    }
    //扣材料
    bool success = trySubMaterial(sp, m_bag);
    if (!success)
    {
        //cout<<"trySubMaterial() return false"<<endl;
        return HC_ERROR;
    }
    //扣卷轴
    int err_code = 0;
    m_bag.addGem(sp->m_gem_id, -1, err_code);
    if (cost_gold)
    {
        if (addGold(-iUpgradeEquiptmentKeepLevelGold) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(m_id, m_ip_address, iUpgradeEquiptmentKeepLevelGold, gold_cost_for_upgrade_equipment, m_union_id, m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(this,iUpgradeEquiptmentKeepLevelGold,gold_cost_for_upgrade_equipment);
#endif
        NotifyCharData();
        //act统计
        act_to_tencent(this,act_new_equipt_make_special,src_eq->qLevel,src_eq->quality);
    }
    //升级装备
    src_eq->upgrade(cost_gold);

    if (src_eq->getContainer() != &m_bag)
    {
        m_weapon_attack_change = true;
        if (pbag->gd.get())
        {
            pbag->gd->updateEquipmentEffect();
        }
    }
    
    json_spirit::Object eq;
    src_eq->toObj(eq);
    robj.push_back( Pair("equipVO", eq) );

    updateEnhanceCost();
    updateEnhanceCDList();
    if (src_eq->baseEq.get())
    {
        std::string link_name = strEquipLink;
        str_replace(link_name, "$G", LEX_CAST_STR(src_eq->id));
        str_replace(link_name, "$C", LEX_CAST_STR(m_id));
        str_replace(link_name, "$N", src_eq->baseEq->name);

        addColor(link_name, src_eq->quality);
        //广播升级装备信息
        std::string notify_msg = strEquiptMakeInfo;
        str_replace(notify_msg, "$N", MakeCharNameLink(m_name));
        str_replace(notify_msg, "$G", link_name);
        GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
    }
    
    //七日目标
    Singleton<seven_Goals_mgr>::Instance().updateGoals(*this,queryCreateDays(),goals_type_equipt,src_eq->baseid);
    
    m_trunk_tasks.updateTask(task_equipment_make, src_eq->baseid);

    
    //act统计
    act_to_tencent(this,act_new_equipt_make,src_eq->type);

    return HC_SUCCESS;
}

//获取装备升级信息
int CharData::getUpgradeEquipmentInfo(int eid, int type, json_spirit::Object& robj)
{
    equipment_scroll* sp = NULL;

    EquipmentData* src_eq = m_bag.getEquipById(eid);
    if (!src_eq)
    {
        src_eq = m_generals.getEquipById(eid);
    }
    if (src_eq)
    {
        sp = Singleton<equipment_scroll_mgr>::Instance().getScrollBySrcId(src_eq->baseid);
        if (!sp)
        {
            return HC_ERROR;
        }
    }
    else
    {
        return HC_ERROR;
    }
    if (sp->m_equipment->needLevel > m_level)
    {
        robj.push_back( Pair("needLevel", sp->m_equipment->needLevel) );
        return HC_SUCCESS;
    }
    //直接制造结果
    json_spirit::Object eq = sp->m_eqobj;
    int nextqLevel = equipmentUpgrade::getInstance()->convertUpgradeLevel(src_eq->quality, src_eq->type, src_eq->qLevel);
    int nextAdd = 0;
    int nextAdd2 = 0;
    upgradeValue(sp->m_equipment->up_quality, src_eq->type, nextqLevel, nextAdd, nextAdd2);
    eq.push_back( Pair("level", nextqLevel) );
    eq.push_back( Pair("addNums", sp->m_equipment->baseValue + nextAdd) );
    eq.push_back( Pair("needLevel", sp->m_equipment->needLevel) );
    if (nextAdd2 > 0)
    {
        eq.push_back( Pair("addNums2", sp->m_equipment->baseValue2 + nextAdd2) );
    }
    //保留等级制造结果
    int nextqLevel_gold = src_eq->qLevel;
    nextAdd = 0;
    nextAdd2 = 0;
    upgradeValue(sp->m_equipment->up_quality, src_eq->type, nextqLevel_gold, nextAdd, nextAdd2);
    eq.push_back( Pair("level_gold", nextqLevel_gold) );
    eq.push_back( Pair("addNums_gold", sp->m_equipment->baseValue + nextAdd) );
    if (nextAdd2 > 0)
    {
        eq.push_back( Pair("addNums2_gold", sp->m_equipment->baseValue2 + nextAdd2) );
    }
    robj.push_back( Pair("eq", eq) );
    
    robj.push_back( Pair("eid", src_eq->id) );

    bool canMake = true;

    json_spirit::Array mlist;

    //type=1显示装备type=2显示卷轴
    if (type == 1)
    {
        json_spirit::Object o;
        o.push_back( Pair("name", sp->m_equipment_src->name) );
        o.push_back( Pair("type", iItem_type_equipment) );
        o.push_back( Pair("id", sp->m_equipment_src->baseid) );
        o.push_back( Pair("spic", sp->m_equipment_src->baseid) );
        o.push_back( Pair("quality", sp->m_equipment_src->quality) );
        o.push_back( Pair("need", 1) );
        o.push_back( Pair("num", 1) );
        if (sp->m_equipment_src->m_place.get())
        {
            o.push_back( Pair("info", sp->m_equipment_src->m_place->info) );
        }
        mlist.push_back(o);
    }

    //检查材料是否足够
    canMake &= checkMaterial(sp, *this, m_bag, mlist, type);

    robj.push_back( Pair("mlist", mlist) );
    
    robj.push_back( Pair("canMake", canMake) );
    return HC_SUCCESS;
}

void CharData::getScrollTips(baseTreasure* bt, json_spirit::Object& robj)
{
    //cout<<"getScrollTips() "<<endl;
    equipment_scroll* sp = Singleton<equipment_scroll_mgr>::Instance().getScroll(bt->value);
    if (!sp)
    {
        //cout<<"not find, "<<bt->value<<endl;
        return;
    }
    json_spirit::Array mlist;
    json_spirit::Object o;
    o.push_back( Pair("name", sp->m_equipment_src->name) );
    o.push_back( Pair("need", 1) );

    EquipmentData* src_eq = m_bag.getDefaultEquip(sp->m_src_equipment, 999);
    if (src_eq)
    {
        o.push_back( Pair("num", 1) );
    }
    else
    {
        o.push_back( Pair("num", 0) );
    }
    mlist.push_back(o);

    //检查材料是否足够
    checkMaterial(sp, *this, m_bag, mlist);

    robj.push_back( Pair("mlist", mlist) );
    if (sp->m_equipment.get())
    {
        o.clear();
        json_spirit::Array canMake;//用数组是为了跟材料tips结构统一
        o.push_back( Pair("type", iItem_type_equipment) );
        o.push_back( Pair("quality", sp->m_equipment->quality) );
        o.push_back( Pair("level", sp->m_equipment->needLevel) );
        o.push_back( Pair("name", sp->m_equipment->name) );
        canMake.push_back(o);
        robj.push_back( Pair("canMake", canMake) );
    }
    return;
}

//打开物品
int CharData::openSlotItm(int slot, int nums, json_spirit::Object& robj)
{
    boost::shared_ptr<iItem> itm = m_bag.getItem(slot);
    if (!itm.get())
    {
        return HC_ERROR;
    }
    if (itm->getType() != iItem_type_gem)
    {
        return HC_ERROR;
    }
    int use_num = itm->getCount();
    if (nums > 0 && itm->getCount() > nums)
    {
        use_num = nums;
    }
    Gem* pg = dynamic_cast<Gem*>(itm.get());
    robj.push_back( Pair("use", pg->getUsage()) );

    switch (pg->getUsage())
    {
        //变身卡
        case ITEM_USAGE_CHANGE_CARD:
        {
            m_change_spic = itm->getSubtype() - 499;
            m_change_spic_time = time(NULL) + 7200;
            m_bag.removeItem(slot);
            itm->Clear();
            itm->Save();
            //robj.push_back( Pair("refresh", 1) );
            setExtraData(char_data_type_normal, char_data_change_spic, m_change_spic);
            setExtraData(char_data_type_normal, char_data_change_spic_time, m_change_spic_time);
            return HC_SUCCESS;
        }
        //兑换货币
        case ITEM_USAGE_SILVER_CARD:
        {
            int total = pg->getValue() * use_num;
            addSilver(total);
            //银币统计
            add_statistics_of_silver_get(m_id,m_ip_address,total,silver_get_by_treasure, m_union_id, m_server_id);
            json_spirit::Object obj;
            obj.push_back( Pair("type", item_type_silver) );
            obj.push_back( Pair("count", total) );
            obj.push_back( Pair("id", 0) );
            obj.push_back( Pair("fac", 0) );
            obj.push_back( Pair("name", strSilver) );
            robj.push_back( Pair("get", obj) );
            if (itm->getCount() > use_num)
            {
                itm->addCount(-use_num);
            }
            else
            {
                m_bag.removeItem(slot);
                itm->Clear();
            }
            itm->Save();
            NotifyCharData();
            return HC_SUCCESS;
        }
        case ITEM_USAGE_GOLD_CARD:
        {
            int total = pg->getValue() * use_num;
            addGold(total);
            //金币获得统计
            add_statistics_of_gold_get(m_id,m_ip_address,total,gold_get_treasure, m_union_id, m_server_id);
            json_spirit::Object obj;
            obj.push_back( Pair("type", item_type_gold) );
            obj.push_back( Pair("count", total) );
            obj.push_back( Pair("id", 0) );
            obj.push_back( Pair("fac", 0) );
            obj.push_back( Pair("name", strGold) );
            robj.push_back( Pair("get", obj) );
            if (itm->getCount() > use_num)
            {
                itm->addCount(-use_num);
            }
            else
            {
                m_bag.removeItem(slot);
                itm->Clear();
            }
            itm->Save();
            NotifyCharData();
            return HC_SUCCESS;
        }
        case ITEM_USAGE_YUSHI_CARD:
        {
            int total = pg->getValue() * use_num;
            addTreasure(treasure_type_yushi, total);
            json_spirit::Object obj;
            obj.push_back( Pair("type", item_type_treasure) );
            obj.push_back( Pair("count", total) );
            obj.push_back( Pair("id", treasure_type_yushi) );
            obj.push_back( Pair("fac", 0) );
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_yushi);
            if (tr.get())
            {
                obj.push_back( Pair("quality", tr->quality) );
                obj.push_back( Pair("spic", tr->spic) );
                obj.push_back( Pair("name", tr->name) );
            }
            robj.push_back( Pair("get", obj) );
            if (itm->getCount() > use_num)
            {
                itm->addCount(-use_num);
            }
            else
            {
                m_bag.removeItem(slot);
                itm->Clear();
            }
            itm->Save();
            return HC_SUCCESS;
        }
        case ITEM_USAGE_PRESTIGE_CARD:
        {
            int total = pg->getValue() * use_num;
            addPrestige(total);
            json_spirit::Object obj;
            obj.push_back( Pair("type", item_type_prestige) );
            obj.push_back( Pair("count", total) );
            obj.push_back( Pair("id", 0) );
            obj.push_back( Pair("fac", 0) );
            obj.push_back( Pair("name", strPrestige) );
            robj.push_back( Pair("get", obj) );
            if (itm->getCount() > use_num)
            {
                itm->addCount(-use_num);
            }
            else
            {
                m_bag.removeItem(slot);
                itm->Clear();
            }
            itm->Save();
            return HC_SUCCESS;
        }
        case ITEM_USAGE_SUPPLY_CARD:
        {
            int total = pg->getValue() * use_num;
            addTreasure(treasure_type_supply, total);
            json_spirit::Object obj;
            obj.push_back( Pair("type", item_type_treasure) );
            obj.push_back( Pair("count", total) );
            obj.push_back( Pair("id", treasure_type_supply) );
            obj.push_back( Pair("fac", 0) );
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
            if (tr.get())
            {
                obj.push_back( Pair("quality", tr->quality) );
                obj.push_back( Pair("spic", tr->spic) );
                obj.push_back( Pair("name", tr->name) );
            }
            robj.push_back( Pair("get", obj) );
            if (itm->getCount() > use_num)
            {
                itm->addCount(-use_num);
            }
            else
            {
                m_bag.removeItem(slot);
                itm->Clear();
            }
            itm->Save();
            NotifyCharData();
            return HC_SUCCESS;
        }
        case ITEM_USAGE_GONGXUN_CARD:
        {
            int total = pg->getValue() * use_num;
            addTreasure(treasure_type_gongxun, total);
            json_spirit::Object obj;
            obj.push_back( Pair("type", item_type_treasure) );
            obj.push_back( Pair("count", total) );
            obj.push_back( Pair("id", treasure_type_gongxun) );
            obj.push_back( Pair("fac", 0) );
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_gongxun);
            if (tr.get())
            {
                obj.push_back( Pair("quality", tr->quality) );
                obj.push_back( Pair("spic", tr->spic) );
                obj.push_back( Pair("name", tr->name) );
            }
            robj.push_back( Pair("get", obj) );
            if (itm->getCount() > use_num)
            {
                itm->addCount(-use_num);
            }
            else
            {
                m_bag.removeItem(slot);
                itm->Clear();
            }
            itm->Save();
            return HC_SUCCESS;
        }
        //卷轴
        case ITEM_USAGE_EQUIPMENT_SCROLL:
        {
            equipment_scroll* sp = Singleton<equipment_scroll_mgr>::Instance().getScroll(pg->getValue());
            if (!sp)
            {
                return HC_ERROR;
            }
            
            json_spirit::Object eq = sp->m_eqobj;

            bool canMake = true;

            json_spirit::Array mlist;

            json_spirit::Object o;
            o.push_back( Pair("name", sp->m_equipment_src->name) );
            o.push_back( Pair("type", iItem_type_equipment) );
            o.push_back( Pair("id", sp->m_equipment_src->baseid) );
            o.push_back( Pair("spic", sp->m_equipment_src->baseid) );
            o.push_back( Pair("quality", sp->m_equipment_src->quality) );
            o.push_back( Pair("need", 1) );
            if (sp->m_equipment_src->m_place.get())
            {
                bool can_sweep = false;
                o.push_back( Pair("info", sp->m_equipment_src->m_place->info) );
                if (sp->m_equipment_src->m_place->type == 1)
                {
                    can_sweep = m_tempo.check_stronghold_can_sweep(sp->m_equipment_src->m_place->mapId,sp->m_equipment_src->m_place->stageId,sp->m_equipment_src->m_place->pos);
                    boost::shared_ptr<StrongholdData> bstronghold = GeneralDataMgr::getInstance()->GetStrongholdData(sp->m_equipment_src->m_place->mapId,sp->m_equipment_src->m_place->stageId,sp->m_equipment_src->m_place->pos);
                    if (bstronghold.get())
                    {
                        o.push_back( Pair("open_level", bstronghold->m_level) );
                    }
                    o.push_back( Pair("can_sweep", can_sweep) );
                }
                else if (sp->m_equipment_src->m_place->type == 2)
                {
                    can_sweep = eliteCombatMgr::getInstance()->check_stronghold_can_attack(m_id,sp->m_equipment_src->m_place->mapId,sp->m_equipment_src->m_place->pos);
                    o.push_back( Pair("can_sweep", can_sweep) );
                }
            }

            EquipmentData* src_eq = m_bag.getDefaultEquip(sp->m_src_equipment, 999);
            if (src_eq)
            {
                robj.push_back( Pair("eid", src_eq->id) );
                o.push_back( Pair("num", 1) );

                int nextqLevel = src_eq->qLevel;
                int nextAdd = src_eq->addValue;
                int nextAdd2 = src_eq->addValue2;
                upgradeValue(sp->m_equipment->up_quality, src_eq->type, nextqLevel, nextAdd, nextAdd2);

                eq.push_back( Pair("level", nextqLevel) );
                eq.push_back( Pair("addNums", sp->m_equipment->baseValue + nextAdd) );

                if (nextAdd2 > 0)
                {
                    eq.push_back( Pair("addNums2", sp->m_equipment->baseValue2 + nextAdd2) );
                }
            }
            else
            {
                eq.push_back( Pair("level", 0) );
                eq.push_back( Pair("addNums", sp->m_equipment->baseValue) );
                if (sp->m_equipment->baseValue2 > 0)
                {
                    eq.push_back( Pair("addNums2", sp->m_equipment->baseValue2) );
                }

                o.push_back( Pair("num", 0) );
                canMake = false;
            }
            mlist.push_back(o);

            robj.push_back( Pair("eq", eq) );

            //检查材料是否足够
            canMake &= checkMaterial(sp, *this, m_bag, mlist);

            robj.push_back( Pair("mlist", mlist) );
            
            robj.push_back( Pair("canMake", canMake) );
            return HC_SUCCESS;
        }
        //VIP经验卡
        case ITEM_USAGE_VIP_EXP_80:
        case ITEM_USAGE_VIP_EXP:
        {
            if (m_vip < 0 || m_vip >= 12)
            {
                return HC_ERROR;
            }
            if (pg->getValue() <= 0)
            {
                return HC_ERROR;
            }
            int add = 0;
            int count = use_num;
            if (count >= itm->getCount())
            {
                count = itm->getCount();
            }
            else if (count <= 0)
            {
                count = 1;
            }
            int old_recharge = m_total_recharge + m_vip_exp;
            //上限限制
            if (pg->getUsage() == ITEM_USAGE_VIP_EXP_80)
            {
                //只能升级到80%
                int max_vip = iVIP_recharge[m_vip] * 4 / 5;
                if (old_recharge >= max_vip)
                {
                    return HC_ERROR_USE_VIP_CARD;
                }
                int canAdd = max_vip - old_recharge;
                int maxUsed = canAdd / pg->getValue();
                if (canAdd % pg->getValue() != 0)
                {
                    ++maxUsed;
                }
                if (count > maxUsed)
                {
                    count = maxUsed;
                }
                add = pg->getValue() * count;
                if (add > canAdd)
                {
                    add = canAdd;
                }
            }
            else if (pg->getUsage() == ITEM_USAGE_VIP_EXP)
            {
                add = pg->getValue() * count;
            }
            if (itm->getCount() == count)
            {
                m_bag.removeItem(slot);
                itm->Clear();
                itm->Save();
            }
            else
            {
                itm->addCount(-count);
                itm->Save();
            }

            m_vip_exp += add;
            InsertSaveDb("update char_data set exp=" + LEX_CAST_STR(m_vip_exp) + " where cid=" + LEX_CAST_STR(m_id));
            if (pg->getUsage() == ITEM_USAGE_VIP_EXP)
            {
                updateVip();
            }

            //通知客户端，充值条变化
            robj.push_back( Pair("rechargeFrom", old_recharge) );
            robj.push_back( Pair("rechargeAdd", add) );

            return HC_SUCCESS;
        }
        //限时增益
        case ITEM_USAGE_BUFF_BINGLI:
        case ITEM_USAGE_BUFF_WUGONG:
        case ITEM_USAGE_BUFF_WUFANG:
        case ITEM_USAGE_BUFF_CEGONG:
        case ITEM_USAGE_BUFF_CEFANG:
        {
            if (pg->getValue() <= 0)
            {
                return HC_ERROR;
            }
            if (itm->getInvalidTime() <= time(NULL))
            {
                m_bag.removeItem(slot);
                itm->Clear();
                itm->Save();
                return HC_ERROR;
            }
            int count = 1;
            if (itm->getCount() == count)
            {
                m_bag.removeItem(slot);
                itm->Clear();
                itm->Save();
            }
            else
            {
                itm->addCount(-count);
                itm->Save();
            }
            m_Buffs.addBuff(pg->getUsage()-ITEM_USAGE_BUFF_BINGLI+1,pg->getValue(),7200);
            #ifdef QQ_PLAT
            treasure_cost_tencent(this,(int)itm->getSubtype(),1);
            #endif
            return HC_SUCCESS;
        }
        //开启宝箱
        case ITEM_USAGE_BOX:
        {
            if (pg->getValue() <= 0)
            {
                return HC_ERROR;
            }
            int count = use_num;
            if (count >= itm->getCount())
            {
                count = itm->getCount();
            }
            else if (count <= 0)
            {
                count = 1;
            }
            //开启宝箱需要银币
            int need_silver = count * pg->getValue();
            if (silver() < need_silver)
            {
                if (count > 1)
                {
                    count = silver() / pg->getValue();
                    need_silver = count * pg->getValue();
                    robj.push_back( Pair("msg", getErrMsg(HC_ERROR_NOT_ENOUGH_SILVER)) );
                }
                else
                {
                    std::string msg = strBoxSilverNotEnoughMsg;
                    str_replace(msg, "$S", LEX_CAST_STR(pg->getValue()));
                    robj.push_back( Pair("msg", msg) );
                    return HC_ERROR;
                }
            }
            //开启宝箱需要位置
            if (count > 0 && (m_bag.size()-m_bag.getUsed()) < count)
                return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
            if (itm->getCount() == count)
            {
                m_bag.removeItem(slot);
                itm->Clear();
                itm->Save();
            }
            else
            {
                itm->addCount(-count);
                itm->Save();
            }
            if (count == 1)
            {
                addSilver(-need_silver);
                add_statistics_of_silver_cost(m_id,m_ip_address,need_silver,silver_cost_for_open_box, m_union_id, m_server_id);
                //给奖励!
                std::list<Item> getItems;
                int notify = lootMgr::getInstance()->getBoxLoots(itm->getSubtype(), getItems, 0);
                if (notify > 0)
                {
                    std::string msg = strBoxGetMsg;
                    str_replace(msg, "$W", MakeCharNameLink(m_name));
                    boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(itm->getSubtype());
                    if (tr.get())
                    {
                        std::string name = tr->name;
                        addColor(name, tr->quality);
                        str_replace(msg, "$B", name);
                    }
                    Item& item = *(getItems.begin());
                    if (item.type == item_type_treasure)
                    {
                        tr = GeneralDataMgr::getInstance()->GetBaseTreasure(item.id);
                        if (tr.get())
                        {
                            std::string name = MakeTreasureLink(tr->name, item.id);
                            addColor(name, tr->quality);
                            str_replace(msg, "$R", name);
                        }
                    }
                    json_spirit::mObject mobj;
                    mobj["cmd"] = "broadCastMsg";
                    mobj["msg"] = msg;
                    boost::shared_ptr<splsTimer> tmsg;
                       tmsg.reset(new splsTimer(4, 1, mobj,1));
                       splsTimerMgr::getInstance()->addTimer(tmsg);
                }
                //随机列表
                std::list<Item> items_list;
                lootMgr::getInstance()->getBoxLootsInfo(itm->getSubtype(),items_list);
                json_spirit::Array alist;
                for (std::list<Item>::iterator it = items_list.begin(); it != items_list.end(); ++it)
                {
                    Item& item = *it;
                    json_spirit::Object obj;
                    if ((*(getItems.begin())).id == item.id && (*(getItems.begin())).type == item.type
                        && (*(getItems.begin())).nums == item.nums && (*(getItems.begin())).fac == item.fac)
                    {
                        obj.push_back( Pair("get", 1) );
                    }
                    item.toObj(obj);
                    alist.push_back(obj);
                }
                robj.push_back( Pair("list", alist) );
                giveLoots(this, getItems, m_area, m_level, 0, NULL, &robj, true, give_box_loot);
            }
            else
            {
                addSilver(-need_silver);
                add_statistics_of_silver_cost(m_id,m_ip_address,need_silver,silver_cost_for_open_box, m_union_id, m_server_id);
                //给奖励!
                std::list<Item> getAllItems;
                for (int i = 0; i < count; ++i)
                {
                    std::list<Item> getItems;
                    int notify = lootMgr::getInstance()->getBoxLoots(itm->getSubtype(), getItems, 0);
                    Item& item = *(getItems.begin());
                    if (notify > 0)
                    {
                        std::string msg = strBoxGetMsg;
                        str_replace(msg, "$W", MakeCharNameLink(m_name));
                        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(itm->getSubtype());
                        if (tr.get())
                        {
                            std::string name = tr->name;
                            addColor(name, tr->quality);
                            str_replace(msg, "$B", name);
                        }
                        if (item.type == item_type_treasure)
                        {
                            tr = GeneralDataMgr::getInstance()->GetBaseTreasure(item.id);
                            if (tr.get())
                            {
                                std::string name = MakeTreasureLink(tr->name, item.id);
                                addColor(name, tr->quality);
                                str_replace(msg, "$R", name);
                            }
                        }
                        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                    }
                    getAllItems.push_back(item);
                }
                giveLoots(this, getAllItems, m_area, m_level, 0, NULL, &robj, true, give_box_loot);
            }
            #ifdef QQ_PLAT
            //treasure_cost_tencent(this,(int)itm->getSubtype(),1);
            #endif
            NotifyCharData();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;//未知类型
}

int EquipmentData::upgrade(bool cost_gold)
{
    if (baseEq.get() && baseEq->m_next.get())
    {
        baseEq = baseEq->m_next;
        baseid = baseEq->baseid;
        quality = baseEq->quality;
        up_quality = baseEq->up_quality;
        value = baseEq->baseValue;
        value2 = baseEq->baseValue2;

        if (!cost_gold)
            qLevel = equipmentUpgrade::getInstance()->convertUpgradeLevel(quality, type, qLevel);
        addValue = 0;
        addValue2 = 0;
        for (int l = 1; l <= qLevel; ++l)
        {
            int add2 = 0;
            int add = equipmentUpgrade::getInstance()->getUpgradeValue(up_quality, type, l, add2);
            addValue += add;
            addValue2 += add2;
        }
        price = 50*quality*(qLevel+1)*(qLevel+20);

        if (getGeneral().get())
        {
            getGeneral()->updateEquipmentEffect();
        }
        m_changed = true;
        setSubType(baseid);
        Save();
        if (quality > 1 && NULL != getChar())
        {
            //制造装备，请好友祝贺 - 蓝色品质及以上
            Singleton<relationMgr>::Instance().postCongradulation(getChar()->m_id, CONGRATULATION_MAKE_EQUIP, baseid, 0);
        }
#ifdef QQ_PLAT
        //首次制作装备分享
        Singleton<inviteMgr>::Instance().update_event(getChar()->m_id, SHARE_EVENT_FIRST_MAKE_EQUIPTMENT, 0);
#endif
    }
}

void EquipmentData::toObj(json_spirit::Object& eq)
{
    eq.push_back( Pair("id", id) );
    eq.push_back( Pair("name", name()) );
    eq.push_back( Pair("spic", baseid) );
    eq.push_back( Pair("type", type) );
    eq.push_back( Pair("level", qLevel) );
    eq.push_back( Pair("maxLevel", quality*20) );

    int cur = getvalue();
    int add2 = 0;
    int add = equipmentUpgrade::getInstance()->getUpgradeValue(up_quality, type, qLevel + 1, add2);
    eq.push_back( Pair("addNums", cur) );
    eq.push_back( Pair("nextNums", cur + add) );

    if (add2 > 0)
    {
        int cur2 = getvalue2();
        eq.push_back( Pair("addNums2", cur2) );
        eq.push_back( Pair("nextNums2", cur2 + add2) );
    }

    eq.push_back( Pair("quality", quality) );
    eq.push_back( Pair("up_quality", up_quality) );
    eq.push_back( Pair("price", sellPrice()) );
    eq.push_back( Pair("memo", memo()) );
    if (baseEq.get())
    {
        eq.push_back( Pair("needLevel", baseEq->needLevel) );
    }
}

//非玩家装备信息
int ProcessGetSysEquipInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
   CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    boost::shared_ptr<baseEquipment> beq = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
    if (beq.get())
    {
        json_spirit::Object eq;
        eq.push_back( Pair("id", id) );
        eq.push_back( Pair("name", beq->name) );
        eq.push_back( Pair("spic", id) );
        eq.push_back( Pair("type", beq->type) );
        eq.push_back( Pair("level", 0) );

        eq.push_back( Pair("addNums", beq->baseValue) );

        if (beq->baseValue2 > 0)
        {
            eq.push_back( Pair("addNums", beq->baseValue2) );
        }
        eq.push_back( Pair("quality", beq->quality) );
        eq.push_back( Pair("price", beq->basePrice) );
        eq.push_back( Pair("memo", beq->desc) );
        eq.push_back( Pair("needLevel", beq->needLevel) );

        robj.push_back( Pair("equipVO", eq) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

