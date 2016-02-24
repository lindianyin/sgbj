
#include "item.h"
#include "data.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_lang.h"
#include "singleton.h"
#include "libao.h"
#include "action.h"

#define INFO(x) cout<<x
extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

//const std::string strNotifyBagFull = "{\"cmd\":\"bag_full\",\"state\":$S,\"s\":200}";

//显示背包
int ProcessShowBag(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    int pageNums = 0;
    READ_INT_FROM_MOBJ(pageNums, o, "pageNums");
    return cdata->m_bag.showBag(page, pageNums, robj);
}

//整理背包
int ProcessSortBag(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    cdata->m_bag.sortBag();
    return HC_SUCCESS;
}

//卖掉物品
int ProcessSellItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int silver_get = 0;
    json_spirit::mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"slot");
    json_spirit::mArray::iterator it = list.begin();
    while (it != list.end())
    {
        if ((*it).type() != json_spirit::obj_type)
        {
            ++it;
            continue;
        }
        json_spirit::mObject& tmp_obj = (*it).get_obj();
        int pos = 0;
        READ_INT_FROM_MOBJ(pos,tmp_obj,"pos");
        boost::shared_ptr<item_base> itm = cdata->m_bag.getItem(pos);
        if (itm.get())
        {
            int silver = itm->sellPrice();
            if (silver > 0)
            {
                silver *= itm->getCount();
                cdata->addSilver(silver, silver_get_sell);
                cdata->m_bag.removeItem(pos);
                itm->Save();
                silver_get += silver;
                cout<<"sell,"<<itm->getType()<<","<<itm->getSubType()<<",price="<<itm->sellPrice()<<endl;
                //卖出道具物品需要更新任务
                switch (itm->getType())
                {
                    case ITEM_TYPE_GEM:
                    {
                        //boost::shared_ptr<baseGem> bg = GeneralDataMgr::getInstance()->GetBaseGem(itm->getSubType());
                        //if (bg.get() && bg->usage == GEM_USAGE_FOR_TASK)
                        {
                            int32_t xcount = cdata->m_bag.getGemCount(itm->getSubType());
                            //cdata->m_trunk_tasks.updateTask(task_get_gem, itm->getSubtype(), xcount);
                            //cout<<"sell,update trank task "<<itm->getSubtype()<<","<<xcount<<endl;
                        }
                        break;
                    }
                    case ITEM_TYPE_EQUIPMENT:
                    {
                        break;
                    }
                    default:
                    {
                        break;
                    }
                }
            }
        }
        ++it;
    }
    if (silver_get > 0)
    {
        robj.push_back( Pair("price", silver_get) );
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//购买背包位
int ProcessBuyBagSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int num = 0;
    READ_INT_FROM_MOBJ(num, o, "num");
    return cdata->m_bag.buyBagSlot(num, robj);
}

//交换背包位
int ProcessSwapBagSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int slot1 = 0, slot2 = 0;
    READ_INT_FROM_MOBJ(slot1, o, "slot1");
    READ_INT_FROM_MOBJ(slot2, o, "slot2");
    if (slot1 != slot2)
    {
        cdata->m_bag.swapItem(slot1, slot2);
    }
    return HC_SUCCESS;
}

//使用背包物品
int ProcessOpenBagSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int slot = 0, nums = 0;
    READ_INT_FROM_MOBJ(slot, o, "slot");
    READ_INT_FROM_MOBJ(nums, o, "num");
    return cdata->m_bag.openSlotItm(slot, nums, robj);
}

//打开背包内礼包
int ProcessOpenBagLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int slot = 0;
    READ_INT_FROM_MOBJ(slot, o, "slot");
    return libaoMgr::getInstance()->openLibao(cdata, slot, robj);
}

//单独请求装备信息
int ProcessGetEquipInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1, cid = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(cid, o, "cid");
    CharData* owner = cdata;
    if (cid != 0)
    {
        owner = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    }
    Equipment* equip = owner->m_bag.getEquipById(id);
    if (!equip)
    {
        equip = owner->m_heros.getEquipById(id);
        if (!equip)
        {
            return HC_ERROR;
        }
    }
    json_spirit::Object eq;
    equip->toObj(eq);
    robj.push_back( Pair("equipVO", eq) );
    return HC_SUCCESS;
}

//单独请求物品信息
int ProcessGetGemInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int type = 1, nums = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    boost::shared_ptr<baseGem> btr = GeneralDataMgr::getInstance()->GetBaseGem(type);
    if (!btr.get())
    {
        ERR();
        return HC_ERROR;
    }

    json_spirit::Object trobj;
    trobj.push_back( Pair("type", btr->id) );
    trobj.push_back( Pair("name", btr->name) );
    trobj.push_back( Pair("spic", btr->spic) );
    trobj.push_back( Pair("quality", btr->quality) );
    int price = btr->sellPrice;
    if (nums > 0)
        price *= nums;
    trobj.push_back( Pair("price", price) );
    trobj.push_back( Pair("memo", btr->memo) );
    robj.push_back( Pair("gemVO", trobj) );
    return HC_SUCCESS;
}

//单独请求礼包信息
int ProcessGetLibaoInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1;
    READ_INT_FROM_MOBJ(id, o, "id");
    return libaoMgr::getInstance()->getLibaoInfo(id, robj);
}

//请求物品列表
int ProcessShowGems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    //窗口展现类型，仅帮客户端储存返回，不处理
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type));
    json_spirit::Array list;
    int cur_nums = 0;
    cdata->m_bag.showBagGem(list, o, cur_nums);
    robj.push_back( Pair("list", list) );
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

//非玩家装备信息
int ProcessGetSysEquipInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 1, quality = 0, level = 0;
    READ_INT_FROM_MOBJ(id, o, "id");
    READ_INT_FROM_MOBJ(quality, o, "quality");
    READ_INT_FROM_MOBJ(level, o, "level");
    boost::shared_ptr<baseEquipment> beq = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
    if (beq.get())
    {
        json_spirit::Object eq;
        eq.push_back( Pair("id", id) );
        eq.push_back( Pair("eid", id) );
        eq.push_back( Pair("name", beq->name) );
        eq.push_back( Pair("spic", id) );
        eq.push_back( Pair("type", beq->type) );
        if (quality > 0)
        {
            eq.push_back( Pair("quality", quality) );
        }
        else
        {
            eq.push_back( Pair("quality", beq->quality) );
            quality = beq->quality;
        }
        if (level > 0)
        {
            double add = 0.0;
            add = (double)beq->value * iEquiptQualityAdd[quality-1];
            int value = beq->value + add * (level-1);
            eq.push_back( Pair("level", level) );
            eq.push_back( Pair("value", value) );
        }
        else
        {
            eq.push_back( Pair("level", level) );
            eq.push_back( Pair("value", beq->value) );
        }
        eq.push_back( Pair("price", beq->sellPrice) );
        eq.push_back( Pair("memo", beq->memo) );
        robj.push_back( Pair("equipVO", eq) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
}

//非玩家装备信息
int ProcessGetSysGemList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    GeneralDataMgr::getInstance()->GetGemList(o, robj);
    return HC_SUCCESS;
}

int ProcessBuyGem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0, nums = 0;
    READ_INT_FROM_MOBJ(id, o, "type");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    if (nums < 1)
        nums = 1;
    if (cdata->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(id);
    if (!bt.get())
    {
        return HC_ERROR;
    }
    if (cdata->m_vip < bt->gold_vip)
    {
        return HC_ERROR_NEED_MORE_VIP;
    }
    if (bt->max_size > 0)
    {
        int left = cdata->m_bag.getSize() - cdata->m_bag.getUsed();
        left *= bt->max_size;
        if (left < nums)
        {
            nums = left;
        }
    }
    int cost_gold = bt->gold_to_buy * nums;
    if (cdata->subGold(cost_gold, gold_cost_buy_gem, true) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //给道具
    cdata->addGem(id, nums, gem_get_buy);
    //通知
    json_spirit::Object getobj;
    Item i(ITEM_TYPE_GEM, id, nums, 0);
    i.toObj(getobj);
    robj.push_back( Pair("get", getobj) );
    return HC_SUCCESS;
}

//查询道具数量
int ProcessGetGemCount(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("nums", cdata->m_bag.getGemCount(id)) );
    return HC_SUCCESS;
}

//查询道具价格
int ProcessGetGemPrice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    robj.push_back( Pair("id", id) );
    boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(id);
    if (!bt.get())
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("sell_price", bt->sellPrice) );
    robj.push_back( Pair("need_gold", bt->gold_to_buy) );
    return HC_SUCCESS;
}

inline bool canMerge(boost::shared_ptr<item_base>& a, boost::shared_ptr<item_base>& b)
{
    if (a.get() && a->getType() == ITEM_TYPE_BAOSHI)
    {
        if (b.get() && ITEM_TYPE_BAOSHI == b->getType() && a->getSubType() == b->getSubType())
        {
            Baoshi* pa = dynamic_cast<Baoshi*>(a.get());
            Baoshi* pb = dynamic_cast<Baoshi*>(b.get());
            return pa->getLevel() == pb->getLevel();
        }
        else
        {
            return false;
        }
    }
    else
    {
        return (a.get() && b.get()
                && a->getType() == b->getType()
                && a->getSubType() == b->getSubType()
                && (a->maxCount() == 0
                    || a->maxCount() > a->getCount()
                    || b->maxCount() > b->getCount()) );
    }
}

//物品比较 1 a比b优先 0a和b相同 -1b比a优先
int8_t itemCompare(item_base& a, item_base& b)
{
    if (a.getType() == b.getType())
    {
        switch (a.getType())
        {
            case ITEM_TYPE_GEM:
                {
                    Gem* pa = dynamic_cast<Gem*>(&a);
                    Gem* pb = dynamic_cast<Gem*>(&b);
                    #if 0
                    //道具里面，材料优先级最高
                    if (pa->getUsage() != pb->getUsage() && pa->getUsage() == GEM_USAGE_EQUIPMENT_METRIAL)
                    {
                        return 1;
                    }
                    //同是材料
                    if (pa->getUsage() == pb->getUsage() && pa->getUsage() == GEM_USAGE_EQUIPMENT_METRIAL)
                    {
                        //判断品质
                        if (pa->getQuality() > pb->getQuality())
                        {
                            return 1;
                        }
                        else if (pa->getQuality() < pb->getQuality())
                        {
                            return -1;
                        }
                    }
                    #endif
                    if (a.getSubType() == b.getSubType())
                    {
                        return 0;
                    }
                    else if (a.getSubType() > b.getSubType())
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                break;
            case ITEM_TYPE_EQUIPMENT:
                {
                    Equipment* pa = dynamic_cast<Equipment*>(&a);
                    Equipment* pb = dynamic_cast<Equipment*>(&b);
                    //先判断品质
                    if (pa->getQuality() > pb->getQuality())
                    {
                        return 1;
                    }
                    else if (pa->getQuality() < pb->getQuality())
                    {
                        return -1;
                    }
                    else
                    {
                        if (pa->getLevel() > pb->getLevel())
                        {
                            return 1;
                        }
                        else if (pa->getLevel() < pb->getLevel())
                        {
                            return -1;
                        }
                        else
                        {
                            if (pa->getSubType() < pb->getSubType())
                            {
                                return -1;
                            }
                            else if (pa->getSubType() > pb->getSubType())
                            {
                                return 1;
                            }
                            else if (pa->getId() < pb->getId())
                            {
                                return 1;
                            }
                            else
                            {
                                return -1;
                            }
                        }
                    }
                }
                break;
        }
    }
    else
    {
        if (a.getType() == ITEM_TYPE_EQUIPMENT)
        {
            return 1;
        }
        else if (b.getType() == ITEM_TYPE_EQUIPMENT)
        {
            return -1;
        }
        else if (a.getType() == ITEM_TYPE_GEM)
        {
            return 1;
        }
        else if (b.getType() == ITEM_TYPE_GEM)
        {
            return -1;
        }
        else
        {
            return -1;
        }
    }
    return 1;
}

std::string Item::name() const
{
    switch (type)
    {
        case ITEM_TYPE_CURRENCY:
            {
                if (id == CURRENCY_ID_GOLD)
                    return strGold;
                else if (id == CURRENCY_ID_SILVER || id == CURRENCY_ID_SILVER_LEVEL)
                    return strSilver;
                else if (id == CURRENCY_ID_EXP)
                    return strExp;
                else if (id == CURRENCY_ID_PRESTIGE1)
                    return strPrestige1;
                else if (id == CURRENCY_ID_PRESTIGE2)
                    return strPrestige2;
                else if (id == CURRENCY_ID_PRESTIGE3)
                    return strPrestige3;
                else if (id == CURRENCY_ID_PRESTIGE4)
                    return strPrestige4;
                else if (id == CURRENCY_ID_PRESTIGE_RANDOM)
                    return strPrestigeRandom;
                else if (id == CURRENCY_ID_CHAR_EXP || id == CURRENCY_ID_CHAR_EXP_LEVEL)
                    return strCharExp;
                else if (id == CURRENCY_ID_ARENA_SCORE)
                    return strArenaScore;
                else if (id == CURRENCY_ID_BIND_GOLD || id == CURRENCY_ID_BIND_GOLD_LEVEL)
                    return strBindGold;
            }
            break;
        case ITEM_TYPE_GEM:
            {
                boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(id);
                if (tr.get())
                {
                    return tr->name;
                }
            }
            break;
        case ITEM_TYPE_HERO_PACK:
            {
                boost::shared_ptr<base_hero_pack> tr = Singleton<HeroMgr>::Instance().GetBaseHeroPack(id);
                if (tr.get())
                {
                    return tr->m_name;
                }
            }
            break;
        case ITEM_TYPE_EQUIPMENT:
            {
                boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
                if (tr.get())
                {
                    return tr->name;
                }
            }
            break;
        case ITEM_TYPE_HERO:
            {
                boost::shared_ptr<baseHeroData> bhd = Singleton<HeroMgr>::Instance().GetBaseHero(id);
                if (bhd.get())
                {
                    return bhd->m_name;
                }
            }
            break;
        case ITEM_TYPE_LIBAO:
            {
                baseLibao* p = libaoMgr::getInstance()->getBaselibao(id);
                if (p != NULL)
                {
                    return p->m_name;
                }
            }
            break;
        case ITEM_TYPE_BAOSHI:
            {
                boost::shared_ptr<baseBaoshi> p = GeneralDataMgr::getInstance()->GetBaseBaoshi(id);
                if (p.get())
                {
                    return p->name;
                }
            }
            break;
    }
    return "unknow";
}

std::string Item::toString(bool withColor, int char_level) const
{
    switch (type)
    {
        case ITEM_TYPE_CURRENCY:
            {
                if (id == CURRENCY_ID_SILVER_LEVEL
                    || id == CURRENCY_ID_CHAR_EXP_LEVEL
                    || id == CURRENCY_ID_BIND_GOLD_LEVEL)
                {
                    return (nums > 1 ? (strSilver + strCounts + LEX_CAST_STR(nums * char_level)) : strSilver);
                }
                else
                {
                    return (nums > 1 ? (name() + strCounts + LEX_CAST_STR(nums)) : name());
                }
            }
            break;
        case ITEM_TYPE_GEM:
            {
                boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(id);
                if (tr.get())
                {
                    if (withColor)
                    {
                        std::string name = (nums > 1 ? (tr->name + strCounts + LEX_CAST_STR(nums)) : tr->name);
                        addColor(name, tr->quality);
                        return name;
                    }
                    else
                    {
                        return (nums > 1 ? (tr->name + strCounts + LEX_CAST_STR(nums)) : tr->name);
                    }
                }
            }
            break;
        case ITEM_TYPE_EQUIPMENT:
            {
                boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
                if (tr.get())
                {
                    if (withColor)
                    {
                        std::string name = (nums > 1 ? (tr->name + strCounts + LEX_CAST_STR(nums)) : tr->name);
                        if (extra > 0)
                        {
                            addColor(name, extra);
                        }
                        else
                        {
                            addColor(name, 1);
                        }
                        return name;
                    }
                    else
                    {
                        return (nums > 1 ? (tr->name + strCounts + LEX_CAST_STR(nums)) :tr->name);
                    }
                }
            }
            break;
        case ITEM_TYPE_HERO:
            {
                boost::shared_ptr<baseHeroData> bhd = Singleton<HeroMgr>::Instance().GetBaseHero(id);
                if (bhd.get())
                {
                    if (withColor)
                    {
                        std::string name = (nums > 1 ? (bhd->m_name + strCounts + LEX_CAST_STR(nums)) : bhd->m_name);
                        if (extra > 0)
                        {
                            addColor(name, extra);
                        }
                        else
                        {
                            addColor(name, 1);
                        }
                        return name;
                    }
                    else
                    {
                        return (nums > 1 ? (bhd->m_name + strCounts + LEX_CAST_STR(nums)) : bhd->m_name);
                    }
                }
            }
            break;
        case ITEM_TYPE_BAOSHI:
            {
                boost::shared_ptr<baseBaoshi> p = GeneralDataMgr::getInstance()->GetBaseBaoshi(id);
                if (p.get())
                {
                    if (withColor)
                    {
                        std::string name = (nums > 1 ? (p->name + strCounts + LEX_CAST_STR(nums)) : p->name);
                        if (extra > 0)
                        {
                            addColor(name, extra);
                        }
                        else
                        {
                            addColor(name, 1);
                        }
                        return name;
                    }
                    else
                    {
                        return (nums > 1 ? (p->name + strCounts + LEX_CAST_STR(nums)) : p->name);
                    }
                }
            }
            break;
        default:
            break;
    }
    return "unknow";
}

void Item::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("type", type) );
    obj.push_back( Pair("count", nums) );
    obj.push_back( Pair("id", id) );
    obj.push_back( Pair("extra", extra) );
    obj.push_back( Pair("extra2", extra2) );
    int quality = 0;
    switch (type)
    {
        case ITEM_TYPE_GEM:
            {
                boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(id);
                if (tr.get())
                {
                    quality = tr->quality;
                    obj.push_back( Pair("spic", tr->spic) );
                }
            }
            break;
        case ITEM_TYPE_HERO_PACK:
            {
                boost::shared_ptr<base_hero_pack> tr = Singleton<HeroMgr>::Instance().GetBaseHeroPack(id);
                if (tr.get())
                {
                    obj.push_back( Pair("spic", tr->m_id) );
                }
            }
            break;
        case ITEM_TYPE_EQUIPMENT:
            {
                boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
                if (tr.get())
                {
                    if (extra > 0)
                    {
                        quality = extra;
                    }
                    else
                    {
                        quality = 1;
                    }
                    obj.push_back( Pair("level", extra2 > 0 ? extra2 : 1));
                    obj.push_back( Pair("spic", tr->spic) );
                }
            }
            break;
        case ITEM_TYPE_HERO:
            {
                boost::shared_ptr<baseHeroData> bhd = Singleton<HeroMgr>::Instance().GetBaseHero(id);
                if (bhd.get())
                {
                    obj.push_back( Pair("spic", bhd->m_spic) );
                    obj.push_back( Pair("race", bhd->m_race));
                    obj.push_back( Pair("star", extra));
                    obj.push_back( Pair("level", extra2 > 0 ? extra2 : 1));
                    obj.push_back( Pair("fac_a", d_extra[0]) );
                    obj.push_back( Pair("fac_b", d_extra[1]) );
                    obj.push_back( Pair("fac_c", d_extra[2]) );
                    obj.push_back( Pair("fac_d", d_extra[3]) );
                    quality = extra;
                }
            }
            break;
        case ITEM_TYPE_LIBAO:
            {
                baseLibao* p = libaoMgr::getInstance()->getBaselibao(id);
                if (p != NULL)
                {
                    quality = p->m_quality;
                    obj.push_back( Pair("spic", p->m_spic) );
                }
            }
            break;
        case ITEM_TYPE_BAOSHI:
        {
            boost::shared_ptr<baseBaoshi> p = GeneralDataMgr::getInstance()->GetBaseBaoshi(id);
            if (p.get())
            {
                if (extra > MAX_BAOSHI_LEVEL || extra < 1)
                {
                    quality = 1;
                }
                else
                {
                    quality = p->qualitys[extra-1];
                }
                obj.push_back( Pair("level", extra) );
            }
            break;
        }
    }
    obj.push_back( Pair("name", name()) );
    if (quality > 0)
    {
        obj.push_back( Pair("quality", quality) );
    }
}

void Item::toPointObj(boost::shared_ptr<json_spirit::Object>& p_obj)
{
    p_obj.reset();
    p_obj.reset(new json_spirit::Object);
    p_obj->push_back( Pair("type", type) );
    p_obj->push_back( Pair("count", nums) );
    p_obj->push_back( Pair("id", id) );
    p_obj->push_back( Pair("extra", extra) );
    p_obj->push_back( Pair("extra2", extra2) );
    int quality = 0;
    switch (type)
    {
        case ITEM_TYPE_GEM:
            {
                boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(id);
                if (tr.get())
                {
                    quality = tr->quality;
                    p_obj->push_back( Pair("spic", tr->spic) );
                }
            }
            break;
        case ITEM_TYPE_HERO_PACK:
            {
                boost::shared_ptr<base_hero_pack> tr = Singleton<HeroMgr>::Instance().GetBaseHeroPack(id);
                if (tr.get())
                {
                    p_obj->push_back( Pair("spic", tr->m_id) );
                }
            }
            break;
        case ITEM_TYPE_EQUIPMENT:
            {
                boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
                if (tr.get())
                {
                    if (extra > 0)
                    {
                        quality = extra;
                    }
                    else
                    {
                        quality = 1;
                    }
                    p_obj->push_back( Pair("level", 1));
                    p_obj->push_back( Pair("spic", tr->spic) );
                }
            }
            break;
        case ITEM_TYPE_HERO:
            {
                boost::shared_ptr<baseHeroData> bhd = Singleton<HeroMgr>::Instance().GetBaseHero(id);
                if (bhd.get())
                {
                    p_obj->push_back( Pair("spic", bhd->m_spic) );
                    p_obj->push_back( Pair("spic", bhd->m_spic) );
                    p_obj->push_back( Pair("race", bhd->m_race));
                    p_obj->push_back( Pair("star", extra));//掉落英雄的时候可以指定星级
                    p_obj->push_back( Pair("level", extra2 > 0 ? extra2 : 1));
                    p_obj->push_back( Pair("fac_a", d_extra[0]) );
                    p_obj->push_back( Pair("fac_b", d_extra[1]) );
                    p_obj->push_back( Pair("fac_c", d_extra[2]) );
                    p_obj->push_back( Pair("fac_d", d_extra[3]) );
                    quality = extra;
                }
            }
            break;
        case ITEM_TYPE_LIBAO:
            {
                baseLibao* p = libaoMgr::getInstance()->getBaselibao(id);
                if (p != NULL)
                {
                    quality = p->m_quality;
                    p_obj->push_back( Pair("spic", p->m_spic) );
                }
            }
            break;
        case ITEM_TYPE_BAOSHI:
        {
            boost::shared_ptr<baseBaoshi> p = GeneralDataMgr::getInstance()->GetBaseBaoshi(id);
            if (p.get())
            {
                if (extra > MAX_BAOSHI_LEVEL || extra < 1)
                {
                    quality = 1;
                }
                else
                {
                    quality = p->qualitys[extra-1];
                }
                p_obj->push_back( Pair("level", extra) );
            }
            break;
        }
    }
    p_obj->push_back( Pair("name", name()) );
    if (quality > 0)
    {
        p_obj->push_back( Pair("quality", quality) );
    }
}

item_base::item_base(uint16_t type, uint16_t subType, uint32_t id, int count)
{
    m_guid.m_type = type;
    m_guid.m_subType = subType;
    m_guid.m_guid = id;
    m_count = count;
    m_container = NULL;
    m_slot = 0;
    m_changed = false;
}

int32_t item_base::addCount(int32_t a)
{
    if ((m_count + a) >= 0)
    {
        m_count += a;
        m_changed = true;
        return m_count;
    }
    else
    {
        return -1;
    }
}

CharData* item_base::getChar() const
{
    if (m_container)
    {
        if (m_container->getType() == BAG_TYPE_CHAR)
        {
            CharBag* tmp = dynamic_cast<CharBag*>(m_container);
            return &tmp->m_chardata;
        }
        else if (m_container->getType() == BAG_TYPE_HERO)
        {
            HeroBag* tmp = dynamic_cast<HeroBag*>(m_container);
            return &tmp->m_herodata.m_belong_to.m_charData;
        }
        else if (m_container->getType() == BAG_TYPE_EQUIPTMENT)
        {
            EquipmentBag* tmp = dynamic_cast<EquipmentBag*>(m_container);
            return tmp->m_equiptdata.getChar();
        }
    }
    return NULL;
}

Gem::Gem(uint16_t subtype, uint32_t id, int32_t count)
:item_base((uint16_t)ITEM_TYPE_GEM, subtype, id, count)
{
    m_base = GeneralDataMgr::getInstance()->GetBaseGem(subtype);
    m_invalidTime = 0;
}

bool Gem::canAuction() const
{
    if (m_base.get())
    {
        return m_base->auction;
    }
    return false;
}

uint16_t Gem::getSpic() const
{
    if (m_base.get())
    {
        return m_base->spic;
    }
    return getSubType();
}

int Gem::getUsage() const
{
    if (m_base.get())
    {
        return m_base->usage;
    }
    return 0;
}

std::string Gem::getName() const
{
    if (m_base.get())
    {
        return m_base->name;
    }
    return "a gem";
}

std::string Gem::getMemo() const
{
    if (m_base.get())
    {
        return m_base->memo;
    }
    return "this is a gem";
}

int Gem::getQuality() const
{
    if (m_base.get())
    {
        return m_base->quality;
    }
    return 1;
}

int32_t Gem::maxCount() const
{
    if (m_base.get())
    {
        return m_base->max_size;
    }
    return 1;
}

int32_t Gem::sellPrice() const
{
    if (m_base.get())
    {
        return m_base->sellPrice;
    }
    return 0;
}

void Gem::setInvalidTime(time_t vt)
{
    m_invalidTime = vt;
    m_changed = true;
}

int Gem::getBaseInvalidTime() const
{
    if (m_base.get())
    {
        return m_base->invalidTime;
    }
    return 0;
}

int Gem::getExtra1() const
{
    if (m_base.get())
    {
        return m_base->extra[0];
    }
    return 1;
}

int Gem::getExtra2() const
{
    if (m_base.get())
    {
        return m_base->extra[1];
    }
    return 1;
}

int Gem::getExtra3() const
{
    if (m_base.get())
    {
        return m_base->extra[2];
    }
    return 1;
}

void Gem::Save()
{
    if (m_changed)
    {
        int32_t c = getCount();
        if (c > 0 && getContainer())
        {
            //保存数据库
            InsertSaveDb("update char_gem set nums=" + LEX_CAST_STR(c) +
                        ",slot=" + LEX_CAST_STR((int)getSlot())
                        + ",invalidTime=" + LEX_CAST_STR(m_invalidTime)
                        + " where id=" + LEX_CAST_STR(getId()));
        }
        else
        {
            //保存数据库
            InsertSaveDb("delete from char_gem where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

void Gem::toObj(json_spirit::Object& gq)
{
    if (!m_base.get())
    {
        return;
    }
    gq.push_back( Pair("id", (int)getId()) );
    gq.push_back( Pair("type", m_base->id) );
    gq.push_back( Pair("spic", m_base->spic) );
    gq.push_back( Pair("quality", m_base->quality) );
    gq.push_back( Pair("name", m_base->name) );
    gq.push_back( Pair("memo", m_base->memo) );
    if (maxCount()  > 1 || maxCount() == 0)
    {
        gq.push_back( Pair("count", getCount()) );
    }
    int price = m_base->sellPrice;
    if (getCount() > 0)
        price *= getCount();
    gq.push_back( Pair("price", price) );
    gq.push_back( Pair("gold", m_base->gold_to_buy) );
    gq.push_back( Pair("level", 1) );
    gq.push_back( Pair("usage", getUsage()) );
    if (getInvalidTime() != 0)
    {
        int left_time = getInvalidTime()-time(NULL);
        gq.push_back( Pair("invalidTime", left_time > 0 ? left_time : 0) );
    }
}

Equipment::Equipment(uint16_t subtype, int id, int cid, int level, int quality)
:item_base((uint16_t)ITEM_TYPE_EQUIPMENT, subtype, id, 1)
,m_bag(*this, 0)
{
    m_base = GeneralDataMgr::getInstance()->GetBaseEquipment(subtype);
    m_cid = cid;
    m_level = level;
    m_quality = quality;
    updateAttribute();
    m_sellPrice = getQuality() > 0 ? 50*getQuality()*(getLevel()+1)*(getLevel()+20) : 50*(getLevel()+1)*(getLevel()+20);
    m_bless_value = 0;
}

uint16_t Equipment::getSpic() const
{
    if (m_base.get())
    {
        return m_base->spic;
    }
    return getSubType();
}

int Equipment::getEquipType() const
{
    if (m_base.get())
    {
        return m_base->type;
    }
    return 0;
}

std::string Equipment::getName() const
{
    if (m_base.get())
    {
        return m_base->name;
    }
    return "a equip";
}

std::string Equipment::getMemo() const
{
    if (m_base.get())
    {
        return m_base->memo;
    }
    return "this is a equip";
}

int Equipment::getCompoundSilver() const
{
    int cost = iCompoundEquiptSilver;
    for(int i = 1; i < m_quality; ++i)
    {
        cost *= 4;
    }
    return cost;
}

void Equipment::updateAttribute(bool real_update)
{
    //根据等级品质基础算出装备加成
    if (m_base.get())
    {
        //每级加成
        double add = 0.0;
        add = (double)m_base->value * iEquiptQualityAdd[m_quality-1];
        m_value = m_base->value + add * (m_level-1);
        //宝石加成
        for (int i = 1; i <= m_bag.m_size; ++i)
        {
            if (m_bag.m_bagslot[i-1].get() && m_bag.m_bagslot[i-1]->getType() == ITEM_TYPE_BAOSHI)
            {
                Baoshi* p = dynamic_cast<Baoshi*>(m_bag.m_bagslot[i-1].get());
                m_value += p->getValue();
            }
        }
        if (real_update)
        {
            Save();
            if (m_container && m_container->getType() == BAG_TYPE_HERO)
            {
                HeroBag* tmp = dynamic_cast<HeroBag*>(m_container);
                //影响英雄加成
                if (tmp->getHero())
                {
                    tmp->getHero()->updateAttribute();
                    if (tmp->getHero()->isDefault())
                    {
                        //通知玩家装备列表变动
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(tmp->getHero()->m_belong_to.m_charData.m_name);
                        if (account.get())
                        {
                            json_spirit::Object robj;
                            robj.push_back( Pair("cmd", "showEquiptList") );
                            robj.push_back( Pair("purpose", 3));
                            robj.push_back( Pair("s", 200) );
                            robj.push_back( Pair("hid", tmp->getHero()->m_id) );
                            json_spirit::Array elists;
                            json_spirit::mObject o;
                            int cur_num = 0;
                            tmp->getHero()->m_bag.showBagEquipments(elists, o, cur_num);
                            robj.push_back( Pair("list", elists) );
                            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
                        }
                    }
                }
            }
        }
    }
}

void Equipment::Save()
{
    if (m_changed)
    {
        if (getContainer())
        {
            CharHeroData* phero = NULL;
            if (m_container->getType() == BAG_TYPE_HERO)
            {
                HeroBag* tmp = dynamic_cast<HeroBag*>(m_container);
                phero = tmp->getHero();
            }
            //保存到数据库
            InsertSaveDb("update char_equipment set level=" + LEX_CAST_STR(m_level)
                    + ",quality="+ LEX_CAST_STR(m_quality)
                    + ",bagSize="+ LEX_CAST_STR(m_bag.getSize())
                    + ",bless_value="+ LEX_CAST_STR(m_bless_value)
                    + ",base_id="+ LEX_CAST_STR(getSubType())
                    + ",slot="+ LEX_CAST_STR((int)getSlot())
                    + ",hid="+ LEX_CAST_STR(phero ? phero->m_id : 0)
                    + " where cid="+ LEX_CAST_STR(m_cid) + " and id=" + LEX_CAST_STR(getId()));
        }
        else
        {
            //保存数据库
            InsertSaveDb("delete from char_equipment where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

void Equipment::toObj(json_spirit::Object& eq, int quality, int level)
{
    if (!m_base.get())
    {
        return;
    }
    //虚拟特定某品质某等级对象
    int org_quality = 0, org_level = 0;
    if (quality != 0)
    {
        org_quality = m_quality;
        m_quality = quality;
    }
    if (level != 0)
    {
        org_level = m_level;
        m_level = level;
    }
    if (quality != 0 || level != 0)
    {
        updateAttribute(false);
    }
    eq.push_back( Pair("id", (int)getId()) );
    eq.push_back( Pair("eid", (int)getSubType()) );
    eq.push_back( Pair("name", getName()) );
    eq.push_back( Pair("spic", getSpic()) );
    eq.push_back( Pair("type", getEquipType()) );
    eq.push_back( Pair("level", m_level) );
    eq.push_back( Pair("value", getValue()) );
    eq.push_back( Pair("quality", getQuality()) );
    eq.push_back( Pair("price", sellPrice()) );
    eq.push_back( Pair("memo", getMemo()) );
    json_spirit::Array list;
    json_spirit::mObject o;
    int tmp = 0;
    m_bag.showBagBaoshis(list, o, tmp);
    eq.push_back( Pair("baoshi_list", list) );
    eq.push_back( Pair("bagSize", m_bag.getSize()) );
    eq.push_back( Pair("bagMaxSize", maxBaoshiSlot()) );
    //虚拟星级后换回正常星级
    if (org_quality != 0)
    {
        m_quality = org_quality;
    }
    if (org_level != 0)
    {
        m_level = org_level;
    }
    if (org_quality != 0 || org_level != 0)
    {
        updateAttribute(false);
    }
}

void Equipment::updateQuality()
{
    if (m_quality < iMaxQuality)
    {
        ++m_quality;
        m_changed = true;
        updateAttribute();
    }
}

void Equipment::updateLevel(int level)
{
    if (level < 1)
        level = 1;
    m_level = level;
    m_changed = true;
    updateAttribute();
    //活动更新
    if (m_container && m_container->getType() == BAG_TYPE_HERO)
    {
        HeroBag* tmp = dynamic_cast<HeroBag*>(m_container);
        if (tmp->m_herodata.isDefault() && Singleton<actionMgr>::Instance().isEquiptLevelActionOpen(getChar()))
        {
            Singleton<actionMgr>::Instance().updateEquiptLevelAction(tmp->getHero());
        }
    }
}

void Equipment::addBlessValue(int value)
{
    m_bless_value += value;
    m_changed = true;
}

void Equipment::clearBlessValue()
{
    m_bless_value = 0;
    m_changed = true;
}

int Equipment::addBaoshiSlot()
{
    CharData* pc = getChar();
    if (pc == NULL)
    {
        return HC_ERROR;
    }
    if (m_bag.getSize() >= maxBaoshiSlot())
    {
        return HC_ERROR;
    }
    int gem_cnt = pc->m_bag.getGemCount(GEM_ID_BAOSHI_SLOT);
    if (gem_cnt < iEuiptBaoshiSlotCost[m_bag.getSize()])
        return HC_ERROR_NOT_ENOUGH_GEM;
    pc->subGem(GEM_ID_BAOSHI_SLOT,iEuiptBaoshiSlotCost[m_bag.getSize()],gem_cost_equipt_add_slot);
    m_bag.addSize(1);
    setChanged();
    Save();
    return HC_SUCCESS;
}

Baoshi::Baoshi(int id, baseBaoshi& base, int level, int count)
:item_base((uint16_t)ITEM_TYPE_BAOSHI, base.id, id, count)
,m_base(base)
{
    m_level = level;
    if (m_level >= 1 && m_level <= MAX_BAOSHI_LEVEL)
    {
        m_quality = m_base.qualitys[m_level-1];
        m_value = m_base.values[m_level-1];
    }
    else
    {
        m_quality = 0;
        m_value = 0;
        m_level = 1;
    }
}

int Baoshi::getBaoshiType() const
{
    return m_base.type;
}

std::string Baoshi::getName() const
{
    return m_base.name;
}

std::string Baoshi::getMemo() const
{
    std::string memo = m_base.memo;
    str_replace(memo, "$V", LEX_CAST_STR(getValue()));
    return memo;
}

void Baoshi::Save()
{
    if (m_changed)
    {
        int32_t c = getCount();
        if (c > 0 && getContainer())
        {
            CharData* pc = getChar();
            Equipment* pe = NULL;
            if (m_container->getType() == BAG_TYPE_EQUIPTMENT)
            {
                EquipmentBag* tmp = dynamic_cast<EquipmentBag*>(m_container);
                pe = tmp->getEquipment();
            }
            InsertSaveDb("update char_baoshi set base_id=" + LEX_CAST_STR(m_base.id)
                                    + ",cid=" + LEX_CAST_STR(pc ? pc->m_id : 0)
                                    + ",slot=" + LEX_CAST_STR((int)(getSlot()))
                                    + ",eid="+ LEX_CAST_STR(pe ? pe->getId() : 0)
                                    + ",level=" + LEX_CAST_STR(getLevel())
                                    + ",nums=" + LEX_CAST_STR(c)
                                    + " where id=" + LEX_CAST_STR(getId()));
        }
        else
        {
            InsertSaveDb("delete from char_baoshi where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

void Baoshi::toObj(json_spirit::Object& obj, int level)
{
    obj.push_back( Pair("id", (int)getId()) );
    obj.push_back( Pair("base_id", (int)getSubType()) );
    obj.push_back( Pair("name", getName()) );
    obj.push_back( Pair("level", (level >= 1 && level <= MAX_BAOSHI_LEVEL) ? level : getLevel()) );
    obj.push_back( Pair("value", (level >= 1 && level <= MAX_BAOSHI_LEVEL) ? m_base.values[level-1] : getValue()) );
    obj.push_back( Pair("quality", getQuality()) );
    obj.push_back( Pair("price", sellPrice()) );
    obj.push_back( Pair("memo", getMemo()) );
    obj.push_back( Pair("count", getCount()) );
    obj.push_back( Pair("slot", getSlot()) );
}

void Baoshi::updateLevel(int level)
{
    if (level >= 1 && level <= MAX_BAOSHI_LEVEL)
    {
        m_level = level;
        m_value = m_base.values[m_level-1];
        m_quality = m_base.qualitys[m_level-1];
        m_changed = true;
    }
}

Bag_base::Bag_base(int type, size_t s)
{
    m_bag_type = type;
    m_size = s;
    m_slot_used = 0;
    m_sorted = false;
}

size_t Bag_base::addSize(size_t a)
{
    if (a + m_size <= MAX_BAG_SIZE)
    {
        m_size += a;
    }
    else
    {
        m_size = MAX_BAG_SIZE;
    }
    return m_size;
}

void Bag_base::swapItem(uint8_t slot1, uint8_t slot2)
{
    if (slot1 != slot2 && slot1 > 0 && slot1 <= m_size
        && slot2 > 0 && slot2 <= m_size)
    {
        m_sorted = false;
        if (m_bagslot[slot1-1].get() && m_bagslot[slot2-1].get())
        {
            //相同的东西
            if (canMerge(m_bagslot[slot2-1], m_bagslot[slot1-1]))
            {
                int32_t c1 = m_bagslot[slot1-1]->getCount();
                int32_t c2 = m_bagslot[slot2-1]->getCount();
                int32_t max_c = m_bagslot[slot2-1]->maxCount();
                if (0 == max_c || (c1+c2) <= max_c)
                {
                    m_bagslot[slot2-1]->addCount(c1);
                    m_bagslot[slot2-1]->Save();
                    m_bagslot[slot1-1]->addCount(-c1);
                    m_bagslot[slot1-1]->Save();
                    removeItem(slot1);
                    return;
                }
                else
                {
                    m_bagslot[slot2-1]->addCount(max_c - c2);
                    m_bagslot[slot1-1]->addCount(c2-max_c);
                    m_bagslot[slot2-1]->Save();
                    m_bagslot[slot1-1]->Save();
                    return;
                }
            }
            //交换
            boost::shared_ptr<item_base> itm1 = m_bagslot[slot1-1];
            m_bagslot[slot1-1] = m_bagslot[slot2-1];
            m_bagslot[slot2-1] = itm1;

            m_bagslot[slot2-1]->setSlot(slot2);
            m_bagslot[slot1-1]->setSlot(slot1);

            m_bagslot[slot1-1]->Save();
            m_bagslot[slot2-1]->Save();
        }
        //slot1->slot2
        else if (m_bagslot[slot1-1].get())
        {
            m_bagslot[slot2-1] = m_bagslot[slot1-1];
            m_bagslot[slot1-1].reset();

            m_bagslot[slot2-1]->setSlot(slot2);
            m_bagslot[slot2-1]->Save();
        }
        else if (m_bagslot[slot2-1].get())
        {
            m_bagslot[slot1-1] = m_bagslot[slot2-1];
            m_bagslot[slot2-1].reset();
            m_bagslot[slot1-1]->setSlot(slot1);
            m_bagslot[slot1-1]->Save();
        }
    }
}

uint8_t Bag_base::addItem(uint8_t slot, boost::shared_ptr<item_base>& itm, bool notify)
{
    //cout<<"Bag::addItem(slot,itm) slot "<<(int)slot<<endl;
    if (itm.get()
        && slot > 0 && slot <= m_size)
    {
        if (!m_bagslot[slot-1].get()
            && !itm->isInBag())
        {
            //cout<<"bag add item slot "<<(int)slot<<endl;
            m_bagslot[slot-1] = itm;
            ++m_slot_used;
            itm->setContainer(this);
            itm->setSlot(slot);
            m_sorted = false;
            return slot;
        }
        else
        {
            //cout<<"Bag::addItem(slot,itm) return 0!"<<endl;
            return 0;
        }
    }
    //cout<<"Bag::addItem(slot,itm) return !!!"<<endl;
    return 0;
}

uint8_t Bag_base::addItem(boost::shared_ptr<item_base>& itm, bool notify)
{
    //cout<<"Bag::addItem(itm) size "<<(m_size)<<endl;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (!m_bagslot[i].get())
        {
            return addItem(i+1, itm, notify);
        }
        //cout<<"Bag::addItem(itm) not empty "<<(i+1)<<endl;
    }
    //cout<<"Bag::addItem(itm) return 0"<<endl;
    return 0;
}

boost::shared_ptr<item_base> Bag_base::removeItem(uint8_t slot, bool notify)
{
    boost::shared_ptr<item_base> itm;
    if (slot > 0 && slot <= m_size)
    {
        if (m_bagslot[slot-1].get())
        {
            m_bagslot[slot-1]->clearContainer();
            itm = m_bagslot[slot-1];
            m_bagslot[slot-1].reset();
            --m_slot_used;
            m_sorted = false;
            return itm;
        }
    }
    return itm;
}

boost::shared_ptr<item_base> Bag_base::getItem(uint8_t slot)
{
    if (slot > 0 && slot <= m_size)
    {
        return m_bagslot[slot-1];
    }
    boost::shared_ptr<item_base> itm;
    return itm;
}

int32_t Bag_base::getCount(uint16_t type, uint16_t subType)
{
    int32_t count = 0;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == type && m_bagslot[i]->getSubType() == subType)
        {
            count += m_bagslot[i]->getCount();
        }
    }
    return count;
}

int Bag_base::showBag(int page, int nums_per_page, json_spirit::Object& robj)
{
    if (page < 1)
    {
        page = 1;
    }
    if (nums_per_page < 1)
        nums_per_page = 45;
    int maxpage = m_size / nums_per_page + 1;
    json_spirit::Array lists;

    if (maxpage > MAX_BAG_SIZE/nums_per_page)
    {
        maxpage = MAX_BAG_SIZE/nums_per_page;
    }

    int first_nums = nums_per_page * (page - 1) + 1;
    int last_nums = nums_per_page * page;

    if (last_nums > MAX_BAG_SIZE)
    {
        first_nums = 1;
        last_nums = nums_per_page;
        page = 1;
    }

    for (int i = first_nums; i <= last_nums; ++i)
    {
        if (i <= m_size && m_bagslot[i-1].get())
        //只算在背包里面的
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_EQUIPMENT)
            {
                Equipment* p = dynamic_cast<Equipment*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", ITEM_TYPE_EQUIPMENT) );
                obj.push_back( Pair("id", (int)p->getId()) );
                obj.push_back( Pair("spic", p->getSpic()) );
                obj.push_back( Pair("type", p->getEquipType()) );
                obj.push_back( Pair("quality", p->getQuality()) );
                obj.push_back( Pair("level", p->getLevel()) );
                obj.push_back( Pair("name", p->getName()) );
                obj.push_back( Pair("price", p->sellPrice()) );
                obj.push_back( Pair("slot", i) );
                lists.push_back(obj);
            }
            else if (pp->getType() == ITEM_TYPE_GEM)
            {
                Gem* p = dynamic_cast<Gem*>(pp);
                if (p->getInvalidTime() != 0 && p->getInvalidTime() <= time(NULL))
                {
                    boost::shared_ptr<item_base> itm = removeItem(i);
                    if (itm.get())
                    {
                        itm->Clear();
                        itm->Save();
                    }
                    continue;
                }
                json_spirit::Object obj;
                obj.push_back( Pair("itype", ITEM_TYPE_GEM) );
                obj.push_back( Pair("slot", i) );
                p->toObj(obj);
                lists.push_back(obj);
            }
            else
            {
                json_spirit::Object obj;
                obj.push_back( Pair("itype", pp->getType()) );
                obj.push_back( Pair("type", pp->getSubType()) );
                obj.push_back( Pair("id", (uint64_t)pp->getId()) );
                obj.push_back( Pair("spic", (uint64_t)pp->getSpic()) );
                obj.push_back( Pair("count", pp->getCount()) );
                obj.push_back( Pair("quality", pp->getQuality()) );
                obj.push_back( Pair("name", pp->getName()) );
                obj.push_back( Pair("price", pp->sellPrice()) );
                if (pp->getType() == ITEM_TYPE_BAOSHI)
                {
                    Baoshi* p = dynamic_cast<Baoshi*>(pp);
                    obj.push_back( Pair("level", p->getLevel()) );
                }
                else
                {
                    obj.push_back( Pair("level", 1) );
                }
                obj.push_back( Pair("slot", i) );
                lists.push_back(obj);
            }
        }
    }

    robj.push_back( Pair("list", lists) );
    robj.push_back( Pair("usedSize", m_slot_used) );
    robj.push_back( Pair("bagSize", m_size) );
    robj.push_back( Pair("buyedSize", m_size - BAG_DEFAULT_SIZE) );

    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

int Bag_base::showBagEquipments(json_spirit::Array& lists, json_spirit::mObject& o, int& cur_nums)
{
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    //筛选条件
    int etype = 0, quality = 0, eid = 0;
    READ_INT_FROM_MOBJ(etype, o, "etype");
    READ_INT_FROM_MOBJ(quality, o, "quality");
    READ_INT_FROM_MOBJ(eid, o, "eid");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 8;
    }
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    std::list<Equipment*> tmp_list;
    tmp_list.clear();
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_EQUIPMENT)
            {
                Equipment* p = dynamic_cast<Equipment*>(pp);
                //有类型筛选
                if (etype != 0 && p->getEquipType() != etype)
                {
                    continue;
                }
				//有基础id筛选
                if (eid != 0 && p->getSubType() != eid)
                {
                    continue;
                }
                //有品质筛选
                if (quality != 0 && p->getQuality() != quality)
                {
                    continue;
                }
                std::list<Equipment*>::iterator it_c = tmp_list.begin();
                while (it_c != tmp_list.end())
                {
                    if ((*it_c)==NULL)
                    {
                        ++it_c;
                        continue;
                    }
                    if ((*it_c)->getEquipType() == p->getEquipType() && (*it_c)->getValue() < p->getValue())
                    {
                        break;
                    }
                    ++it_c;
                }
                tmp_list.insert(it_c,p);
            }
        }
    }
    if (tmp_list.size())
    {
        std::list<Equipment*>::iterator itt = tmp_list.begin();
        while (itt != tmp_list.end())
        {
            if ((*itt) == NULL)
            {
                ++itt;
                continue;
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                (*itt)->toObj(obj);
                lists.push_back(obj);
            }
            ++itt;
        }
    }
    return HC_SUCCESS;
}

int Bag_base::showBagGem(json_spirit::Array& lists, json_spirit::mObject& o, int& cur_nums)
{
    //页面信息
    int page = 1, nums_per_page = 8;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    //筛选条件
    int gtype = 0, usage = 0;
    READ_INT_FROM_MOBJ(gtype, o, "gtype");
    READ_INT_FROM_MOBJ(usage, o, "usage");
    //拍卖行特殊处理
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 8;
    }
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    std::list<Gem*> tmp_list;
    tmp_list.clear();
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_GEM)
            {
                Gem* p = dynamic_cast<Gem*>(pp);
                //有类型筛选
                if (gtype != 0 && pp->getSubType() != gtype)
                {
                    continue;
                }
				//有用途筛选
                if (usage != 0 && p->getUsage() != usage)
                {
                    continue;
                }
                //拍卖行只取部分用途的道具
                if (type == 3 && !p->canAuction())
                {
                    continue;
                }
                std::list<Gem*>::iterator it_c = tmp_list.begin();
                while (it_c != tmp_list.end())
                {
                    if ((*it_c)==NULL)
                    {
                        ++it_c;
                        continue;
                    }
                    if ((*it_c)->getSubType() < p->getSubType())
                    {
                        break;
                    }
                    ++it_c;
                }
                tmp_list.insert(it_c,p);
            }
        }
    }
    if (tmp_list.size())
    {
        std::list<Gem*>::iterator itt = tmp_list.begin();
        while (itt != tmp_list.end())
        {
            if ((*itt) == NULL)
            {
                ++itt;
                continue;
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                (*itt)->toObj(obj);
                lists.push_back(obj);
            }
            ++itt;
        }
    }
    return HC_SUCCESS;
}

int Bag_base::showBagBaoshis(json_spirit::Array& lists, json_spirit::mObject& o, int& cur_nums)
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
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;
    std::list<Baoshi*> tmp_list;
    tmp_list.clear();
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_BAOSHI)
            {
                Baoshi* p = dynamic_cast<Baoshi*>(pp);
                std::list<Baoshi*>::iterator it_c = tmp_list.begin();
                while (it_c != tmp_list.end())
                {
                    if ((*it_c)==NULL)
                    {
                        ++it_c;
                        continue;
                    }
                    if ((*it_c)->getSubType() < p->getSubType())
                    {
                        break;
                    }
                    ++it_c;
                }
                tmp_list.insert(it_c,p);
            }
        }
    }
    if (tmp_list.size())
    {
        std::list<Baoshi*>::iterator itt = tmp_list.begin();
        while (itt != tmp_list.end())
        {
            if ((*itt) == NULL)
            {
                ++itt;
                continue;
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                json_spirit::Object obj;
                (*itt)->toObj(obj);
                lists.push_back(obj);
            }
            ++itt;
        }
    }
    return HC_SUCCESS;
}

Equipment* Bag_base::getEquipById(int id)
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == ITEM_TYPE_EQUIPMENT)
        {
            Equipment* p = dynamic_cast<Equipment*>(m_bagslot[i].get());
            if (p->getId() == id)
            {
                return p;
            }
        }
    }
    return NULL;
}

int Bag_base::bestEquipmentQuality(int etype)
{
    int ret = 0;
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_EQUIPMENT)
            {
                Equipment* p = dynamic_cast<Equipment*>(pp);
                //有类型筛选
                if (etype != 0 && p->getEquipType() != etype)
                {
                    continue;
                }
                //更高品质
                if (p->getQuality() > ret)
                {
                    ret = p->getQuality();
                }
            }
        }
    }
    return ret;
}

int Bag_base::bestEquipmentLevel(int etype)
{
    int ret = 0;
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_EQUIPMENT)
            {
                Equipment* p = dynamic_cast<Equipment*>(pp);
                //有类型筛选
                if (etype != 0 && p->getEquipType() != etype)
                {
                    continue;
                }
                //更高等级
                if (p->getLevel() > ret)
                {
                    ret = p->getQuality();
                }
            }
        }
    }
    return ret;
}

Baoshi* Bag_base::getBaoshiById(int id)
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == ITEM_TYPE_BAOSHI)
        {
            Baoshi* p = dynamic_cast<Baoshi*>(m_bagslot[i].get());
            if (p->getId() == id)
            {
                return p;
            }
        }
    }
    return NULL;
}

CharBag::CharBag(CharData& c, size_t s)
:m_chardata(c)
,Bag_base(BAG_TYPE_CHAR, s)
{
}

int32_t CharBag::addGem(uint16_t stype, int32_t count, int& err_code, bool sell_if_full)
{
    if (count == 0 || stype == 0)
    {
        return 0;
    }
    boost::shared_ptr<baseGem> bg = GeneralDataMgr::getInstance()->GetBaseGem(stype);
    if (!bg.get() || bg->currency)
    {
        return 0;
    }
    if (count > 0)
    {
        m_sorted = false;
        int32_t max_c = bg->max_size;
        if (max_c > 1 || 0 == max_c)
        {
            for (size_t i = 0; i < m_size && count > 0; ++i)
            {
                item_base* p = m_bagslot[i].get();
                if (p && p->getType() == ITEM_TYPE_GEM && p->getSubType() == stype)
                {
                    int32_t c = p->getCount();
                    if (0 == max_c || (c + count) <= max_c)
                    {
                        p->addCount(count);
                        count = 0;
                    }
                    else
                    {
                        p->addCount(max_c - c);
                        count = count + c - max_c;
                    }
                    InsertSaveDb("update char_gem set nums=" + LEX_CAST_STR(p->getCount())+ " where id=" + LEX_CAST_STR(p->getId()));
                    p->unsetChanged();
                }
            }
        }
        if (count > 0)
        {
            for (size_t i = 0; i < m_size && count > 0; ++i)
            {
                item_base* p = m_bagslot[i].get();
                if (!p)
                {
                    //增加道具
                    boost::shared_ptr<item_base> t;
                    uint32_t id = GeneralDataMgr::getInstance()->newGemId();
                    if (0 == max_c || count <= max_c)
                    {
                        t.reset(dynamic_cast<item_base*>(new Gem(stype, id, count)));
                        count = 0;
                    }
                    else
                    {
                        t.reset(dynamic_cast<item_base*>(new Gem(stype, id, max_c)));
                        count -= max_c;
                    }
                    Gem* pg = dynamic_cast<Gem*>(t.get());
                    if (pg->getBaseInvalidTime())
                    {
                        pg->setInvalidTime(pg->getBaseInvalidTime()*60 + time(NULL));
                    }
                    addItem(i + 1, t);

                    //保存数据库
                    InsertSaveDb("insert into char_gem (id,cid,tid,nums,slot,invalidTime) value ("
                        + LEX_CAST_STR(id) + ","
                        + LEX_CAST_STR(m_chardata.m_id) + ","
                        + LEX_CAST_STR(stype) + ","
                        + LEX_CAST_STR(t->getCount()) + ","
                        + LEX_CAST_STR(i+1) + ","
                        + LEX_CAST_STR(pg->getInvalidTime()) + ")");
                    t->unsetChanged();
                }
            }
            if (count > 0)
                err_code = HC_ERROR_BAG_FULL;
        }
        //通知客户端
        json_spirit::Object obj;
        json_spirit::Object item;
        item.push_back( Pair("spic", bg->spic) );
        item.push_back( Pair("type", ITEM_TYPE_GEM) );
        obj.push_back( Pair("cmd", "notifyGet") );
        obj.push_back( Pair("item", item) );
        obj.push_back( Pair("s", 200) );
        m_chardata.sendObj(obj);
        return getGemCount(stype);
    }
    else
    {
        int32_t cur_c = getGemCount(stype);
        if ((cur_c + count) < 0)
        {
            return -1;
        }
        m_sorted = false;
        for (size_t i = 0; i < m_size && count < 0; ++i)
        {
            item_base* p = m_bagslot[i].get();
            if (p && p->getType() == ITEM_TYPE_GEM && p->getSubType() == stype)
            {
                int32_t c = p->getCount();
                if (c + count >= 0)
                {
                    //扣
                    p->addCount(count);
                    count = 0;
                }
                else
                {
                    //扣
                    p->addCount(-c);
                    count += c;
                }
                if (p->getCount() == 0)
                {
                    InsertSaveDb("delete from char_gem where id=" + LEX_CAST_STR(p->getId()));
                    removeItem(i+1);
                }
                else
                {
                    p->Save();
                }
            }
        }
        return getGemCount(stype);
    }
}

int32_t CharBag::getGemCurrency(uint16_t stype)
{
    Currency& c = m_chardata.m_currencys[stype];
    return c.count;
}

int32_t CharBag::getGemCount(uint16_t stype)
{
    boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(stype);
    if (!bt.get())
    {
        return 0;
    }
    if (bt->currency)
    {
        return getGemCurrency(stype);
    }
    else
    {
        return getCount((uint16_t) ITEM_TYPE_GEM, stype);
    }
}

//背包整理
void CharBag::sortBag()
{
    if (!m_sorted)
    {
        m_sorted = true;
        boost::shared_ptr<item_base> bagslot[MAX_BAG_SIZE];
        for (int i = 0; i < m_size; ++i)
        {
            bagslot[i] = m_bagslot[i];
            m_bagslot[i].reset();
        }
        for (int i = 0; i < m_size; ++i)
        {
            if (bagslot[i].get())
            {
                sortInsert(bagslot[i]);
            }
        }
        m_slot_used = 0;
        for (int i = 0; i < m_size; ++i)
        {
            if (m_bagslot[i].get())
            {
                m_bagslot[i]->Save();
                if (m_bagslot[i]->getCount() == 0)
                {
                    m_bagslot[i].reset();
                }
                else
                {
                    ++m_slot_used;
                }
            }
        }
    }
}

//排序插入
void CharBag::sortInsert(boost::shared_ptr<item_base>& itm)
{
    if (!itm.get())
    {
        return;
    }
    for (int i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get())
        {
            switch (itemCompare(*(itm.get()), *(m_bagslot[i].get())))
            {
                case 1:
                    //优先级高，插入
                    for (int j = m_size - 1; j > i && j >= 1; --j)
                    {
                        m_bagslot[j] = m_bagslot[j-1];
                        if (m_bagslot[j].get() && m_bagslot[j]->getSlot() != (j+1))
                        {
                            m_bagslot[j]->setSlot(j+1);
                        }
                    }
                    m_bagslot[i] = itm;
                    if (itm->getSlot() != (i+1))
                    {
                        itm->setSlot(i+1);
                    }
                    return;
                case -1:
                    //优先级低的直接跳过
                    break;
                case 0:
                    //优先级相同，可以合并否
                    if (itm->getType() == ITEM_TYPE_GEM)
                    {
                        int max_c = m_bagslot[i]->maxCount();
                        int canAdd = max_c == 0 ? itm->getCount() : (m_bagslot[i]->maxCount() - m_bagslot[i]->getCount());
                        if (canAdd > 0)
                        {
                            if (canAdd > itm->getCount())
                            {
                                canAdd = itm->getCount();
                            }
                            m_bagslot[i]->addCount(canAdd);
                            itm->addCount(-canAdd);
                            if (itm->getCount() == 0)
                            {
                                itm->Save();
                                return;
                            }
                        }
                    }
                    else
                    {
                        ERR();
                        //跳过
                    }
                    break;
            }
        }
        else
        {
            m_bagslot[i] = itm;
            if (itm->getSlot() != (i+1))
            {
                itm->setSlot(i+1);
            }
            return;
        }
    }
}

//购买格子
int CharBag::buyBagSlot(int num, json_spirit::Object& robj)
{
    if (num < 0)
    {
        num = 1;
    }
    if (num + getSize() > MAX_BAG_SIZE)
    {
        num = MAX_BAG_SIZE - getSize();
        if (num < 1)
        {
            return HC_ERROR;
        }
    }

    int buyed = getSize() - BAG_DEFAULT_SIZE;
    int gold_need = 0;
    for (int i = buyed + 1; i <= (buyed+num); ++i)
    {
        int tmp = ((i-1) / 3 + 1) * 2;
        if (tmp >= 40)
            tmp = 40;
        gold_need += tmp;
    }
    if (m_chardata.subGold(gold_need, gold_cost_buy_bag) >= 0)
    {
        addSize(num);
        //保存背包购买
        InsertSaveDb("update char_data set bagSize=" + LEX_CAST_STR(buyed+num) + " where cid=" + LEX_CAST_STR(m_chardata.m_id));
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
}

void CharBag::loadBag()
{
    Query q(GetDb());
    q.get_result("select id,base_id,level,quality,bagSize,bless_value,slot from char_equipment where hid=0 and cid=" + LEX_CAST_STR(m_chardata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int baseid = q.getval();
        int level = q.getval();
        int quality = q.getval();
        int bagSize = q.getval();
        int bless_value = q.getval();
        boost::shared_ptr<baseEquipment> pe = GeneralDataMgr::getInstance()->GetBaseEquipment(baseid);
        if (!pe.get())
        {
            ERR();
            INFO("error base_equipt base_id=" << baseid);
            continue;
        }
        boost::shared_ptr<item_base> e;
        Equipment* eq = new Equipment(baseid, id, m_chardata.m_id, level,quality);
        eq->addBlessValue(bless_value);
        e.reset(eq);
        uint8_t slot = q.getval();
        if (slot >= 1 && slot <= m_size && !getItem(slot).get())
        {
            addItem(slot, e);
        }
        else
        {
            slot = addItem(e);
            e->Save();
        }
        eq->m_bag.setSize(bagSize);
        eq->m_bag.loadBag();
    }
    q.free_result();

    time_t t_now = time(NULL);
    q.get_result("select tid,nums,slot,id,invalidTime from char_gem where cid=" + LEX_CAST_STR(m_chardata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        uint16_t tid = q.getval();
        boost::shared_ptr<baseGem> pg = GeneralDataMgr::getInstance()->GetBaseGem(tid);
        if (!pg.get())
        {
            ERR();
            INFO("error gem id "<<tid<<",cid:"<<m_chardata.m_id);
            continue;
        }
        int32_t count = q.getval();
        uint8_t slot = q.getval();
        uint32_t id = q.getval();
        time_t invalidTime = q.getval();

        if (pg->currency)
        {
            Currency& cc = m_chardata.m_currencys[tid];
            cc.id = id;
            cc.type = tid;
            cc.count = count;
        }
        else
        {
            if (invalidTime == 0 || invalidTime > t_now)
            {
                boost::shared_ptr<item_base> t;
                Gem* g = new Gem(tid, id, count);
                t.reset(g);
                g->setInvalidTime(invalidTime);
                if (slot >= 1 && slot <= m_size && !getItem(slot).get())
                {
                    addItem(slot, t);
                }
                else
                {
                    slot = addItem(t);
                    t->Save();
                }
            }
        }
    }
    q.free_result();

    q.get_result("select libao_id,slot,id from char_libao where cid=" + LEX_CAST_STR(m_chardata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        uint16_t libao_id = q.getval();
        uint8_t slot = q.getval();
        int id = q.getval();
        cout << libao_id << "," << slot << "," << id << endl;
        baseLibao* p = libaoMgr::getInstance()->getBaselibao(libao_id);
        if (p)
        {
            boost::shared_ptr<item_base> t;
            t.reset(new libao(id, *p));
            if (slot >= 1 && slot <= m_size && !getItem(slot).get())
            {
                addItem(slot, t);
            }
            else
            {
                slot = addItem(t);
                t->Save();
            }
        }
        else
        {
            ERR();
            INFO("error libao id "<<libao_id<<",cid:"<<m_chardata.m_id);
        }
    }
    q.free_result();

    q.get_result("select id,base_id,slot,level,nums from char_baoshi where eid=0 and cid=" + LEX_CAST_STR(m_chardata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int base_id = q.getval();
        int slot = q.getval();
        int level = q.getval();
        int nums = q.getval();

        boost::shared_ptr<baseBaoshi> pb = GeneralDataMgr::getInstance()->GetBaseBaoshi(base_id);
        if (!pb.get())
        {
            ERR();
            INFO("error base_baoshi base_id=" << base_id);
            continue;
        }
        boost::shared_ptr<item_base> e;
        Baoshi* eq = new Baoshi(id, *(pb.get()), level, nums);
        e.reset(eq);
        if (slot >= 1 && slot <= m_size && !getItem(slot).get())
        {
            addItem(slot, e);
        }
        else
        {
            slot = addItem(e);
            e->Save();
        }
    }
    q.free_result();
    return;
}

boost::shared_ptr<item_base> CharBag::getEquipItemById(int id)
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == ITEM_TYPE_EQUIPMENT)
        {
            Equipment* p = dynamic_cast<Equipment*>(m_bagslot[i].get());
            if (p->getId() == id)
            {
                return m_bagslot[i];
            }
        }
    }
    boost::shared_ptr<item_base> nil;
    return nil;
}

//打开物品
int CharBag::openSlotItm(int slot, int nums, json_spirit::Object& robj)
{
    boost::shared_ptr<item_base> itm = getItem(slot);
    if (!itm.get())
    {
        return HC_ERROR;
    }
    if (itm->getType() != ITEM_TYPE_GEM)
    {
        return HC_ERROR;
    }
    int use_num = itm->getCount();
    if (nums > 0 && itm->getCount() > nums)
    {
        use_num = nums;
    }
    Gem* pg = dynamic_cast<Gem*>(itm.get());
    robj.push_back( Pair("usage", pg->getUsage()) );
    switch (pg->getUsage())
    {
        case GEM_USAGE_PRESTIGE:
        {
            int race = pg->getExtra1();
            int total = pg->getExtra2() * use_num;
            m_chardata.addPrestige(race, total, prestige_get_gem);
            json_spirit::Object obj;
            Item item(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN + race, total, 0);
            item.toObj(obj);
            robj.push_back( Pair("get", obj) );
            m_chardata.subGem(pg->getSubType(),use_num,gem_cost_use);
            return HC_SUCCESS;
        }
        case GEM_USAGE_SILVER:
        {
            int total = pg->getExtra1() * use_num;
            m_chardata.addSilver(total, silver_get_gem);
            json_spirit::Object obj;
            Item item(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, total, 0);
            item.toObj(obj);
            robj.push_back( Pair("get", obj) );
            m_chardata.subGem(pg->getSubType(),use_num,gem_cost_use);
            return HC_SUCCESS;
        }
        case GEM_USAGE_VIP_EXP:
        {
            if (m_chardata.m_vip < 0 || m_chardata.m_vip >= iMaxVIP)
            {
                return HC_ERROR;
            }
            if (pg->getExtra1() <= 0)
            {
                return HC_ERROR;
            }
            int add = 0;
            int count = use_num;
            if (count <= 0)
            {
                count = 1;
            }
            int old_recharge = m_chardata.m_total_recharge + m_chardata.m_vip_exp;
            //上限限制
            if (pg->getSubType() == GEM_ID_VIP_CARD_80)
            {
                //只能升级到90%
                int max_vip = iVIP_recharge[m_chardata.m_vip] * 9 / 10;
                if (old_recharge >= max_vip)
                {
                    return HC_ERROR_USE_VIP_CARD;
                }
                int canAdd = max_vip - old_recharge;
                int maxUsed = canAdd / pg->getExtra1();
                if (canAdd % pg->getExtra1() != 0)
                {
                    ++maxUsed;
                }
                if (count > maxUsed)
                {
                    count = maxUsed;
                }
                add = pg->getExtra1() * count;
                if (add > canAdd)
                {
                    add = canAdd;
                }
            }
            else if (pg->getSubType() == GEM_ID_VIP_CARD)
            {
                add = pg->getExtra1() * count;
            }
            m_chardata.subGem(pg->getSubType(),use_num,gem_cost_use);
            m_chardata.m_vip_exp += add;
            InsertSaveDb("update char_data set vip_exp=" + LEX_CAST_STR(m_chardata.m_vip_exp) + " where cid=" + LEX_CAST_STR(m_chardata.m_id));
            if (pg->getSubType() == GEM_ID_VIP_CARD)
            {
                m_chardata.updateVip();
            }
            //通知客户端，充值条变化
            robj.push_back( Pair("rechargeFrom", old_recharge) );
            robj.push_back( Pair("rechargeAdd", add) );
            return HC_SUCCESS;
        }
        //限时增益
        case GEM_USAGE_BUFF:
        {
            if (pg->getExtra1() <= 0 || pg->getExtra2() <= 0 || pg->getExtra3() <= 0)
            {
                return HC_ERROR;
            }
            if (pg->getInvalidTime() <= time(NULL))
            {
                removeItem(slot);
                itm->Clear();
                itm->Save();
                return HC_ERROR;
            }
            m_chardata.subGem(pg->getSubType(),1,gem_cost_use);
            m_chardata.m_Buffs.addBuff(pg->getExtra1(),pg->getExtra2(),pg->getExtra3());
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;//未知类型
}

int CharBag::getChengzhangLibaoSlot(int libao_id)
{
    for (int i = 1; i <= m_size; ++i)
    {
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_LIBAO)
            {
                libao* pl = dynamic_cast<libao*>(pp);
                if (pl && pl->m_base.m_libao_id == libao_id && libaoMgr::getInstance()->isChengzhangLibao(pl->m_base.m_libao_id))
                {
                    return i;
                }
            }
        }
    }
    return 0;
}

void CharBag::getBagEquipments(int etype, int quality, int eid, std::list<int>& id_list)
{
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_EQUIPMENT)
            {
                Equipment* p = dynamic_cast<Equipment*>(pp);
                //有类型筛选
                if (etype != 0 && p->getEquipType() != etype)
                {
                    continue;
                }
				//有基础id筛选
                if (eid != 0 && p->getSubType() != eid)
                {
                    continue;
                }
                //有品质筛选
                if (quality != 0 && p->getQuality() != quality)
                {
                    continue;
                }
                id_list.push_back(p->getId());
            }
        }
    }
    return;
}

Baoshi* CharBag::getBaoshiCanMerge(int base_id, int level, int count)
{
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            item_base* pp = m_bagslot[i-1].get();
            if (pp->getType() == ITEM_TYPE_BAOSHI)
            {
                Baoshi* pb = dynamic_cast<Baoshi*>(pp);
                if (pb->getSubType() == base_id && pb->getLevel() == level)
                {
                    if (pp->getCount() + count <= MAX_BAOSHI_COUNT)
                    {
                        //cout<<"getBaoshiCanMerge:slot"<<i<<endl;
                        return pb;
                    }
                }
            }
        }
    }
    return NULL;
}

boost::shared_ptr<item_base> CharBag::cloneBaoshi(int base_id, int level, int count)
{
    boost::shared_ptr<item_base> bs;
    boost::shared_ptr<baseBaoshi> bbs = GeneralDataMgr::getInstance()->GetBaseBaoshi(base_id);
    if (!bbs.get())
    {
         return bs;
    }
    int id = GeneralDataMgr::getInstance()->newBaoshiId();
    if (level < 1 || level > MAX_BAOSHI_LEVEL)
    {
        level = 1;
    }
    Baoshi* pb = new Baoshi(id, *(bbs.get()), level, count);
    bs.reset(pb);
    InsertSaveDb("insert into char_baoshi set base_id=" + LEX_CAST_STR(pb->getSubType())
                                + ",eid=0,slot=" + LEX_CAST_STR((int)(bs->getSlot()))
                                + ",level=" + LEX_CAST_STR(pb->getLevel())
                                + ",cid=0,nums=1,id=" + LEX_CAST_STR((int)(bs->getId())));

    return bs;
}

//增加宝石
int CharBag::addBaoshi(int base_id, int level, int count)
{
    if (isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    Baoshi* p = getBaoshiCanMerge(base_id, level, count);
    if (p)
    {
        p->addCount(count);
        p->Save();
    }
    else
    {
        boost::shared_ptr<item_base> bs = cloneBaoshi(base_id, level, count);
        if (!bs.get())
        {
            return HC_ERROR;
        }
        addItem(bs);
        Baoshi* pb = dynamic_cast<Baoshi*>(bs.get());
        pb->Save();
    }
    //通知客户端
    json_spirit::Object obj;
    json_spirit::Object item;
    item.push_back( Pair("id", base_id) );
    item.push_back( Pair("level", level) );
    item.push_back( Pair("type", ITEM_TYPE_BAOSHI) );
    obj.push_back( Pair("cmd", "notifyGet") );
    obj.push_back( Pair("item", item) );
    obj.push_back( Pair("s", 200) );
    m_chardata.sendObj(obj);
    return HC_SUCCESS;
}

//改变宝石数量
int CharBag::addBaoshiCount(int base_id, int level, int count)
{
    m_sorted = false;
    int oldcount = count;
    if (count > 0)
    {
        for (int i = 1; i <= m_size; ++i)
        {
            //只算在背包里面的
            if (m_bagslot[i-1].get())
            {
                item_base* pp = m_bagslot[i-1].get();
                if (pp->getType() == ITEM_TYPE_BAOSHI)
                {
                    Baoshi* pb = dynamic_cast<Baoshi*>(pp);
                    if (pb->getSubType() == base_id && pb->getLevel() == level)
                    {
                        if (pp->getCount() + count <= pp->maxCount())
                        {
                            pp->addCount(count);
                            pb->Save();
                            //通知客户端
                            json_spirit::Object obj;
                            json_spirit::Object item;
                            item.push_back( Pair("id", base_id) );
                            item.push_back( Pair("level", level) );
                            item.push_back( Pair("type", ITEM_TYPE_BAOSHI) );
                            obj.push_back( Pair("cmd", "notifyGet") );
                            obj.push_back( Pair("item", item) );
                            obj.push_back( Pair("s", 200) );
                            m_chardata.sendObj(obj);
                            return oldcount;
                        }
                        else
                        {
                            count -= (pp->maxCount() - pp->getCount());
                            pp->addCount(pp->maxCount() - pp->getCount());
                            pb->Save();
                        }
                    }
                }
            }
        }
        while (count > 0 && !isFull())
        {
            int bscount = 0;
            if (count > MAX_BAOSHI_COUNT)
            {
                bscount = MAX_BAOSHI_COUNT;
                count -= MAX_BAOSHI_COUNT;
            }
            else
            {
                bscount = count;
                count = 0;
            }

            boost::shared_ptr<item_base> bs = cloneBaoshi(base_id, level, bscount);
            if (bs.get())
            {
                if (0 == addItem(bs))
                {
                    count += bscount;
                    bs->Clear();
                    bs->Save();
                    break;
                }
                else
                {
                    bs->Save();
                }
            }
            else
            {
                cout<<"!!!!!!!!!!!!!! baoshi baseid"<<base_id<<",level:"<<level<<","<<bscount<<endl;
            }
        }
        //通知客户端
        json_spirit::Object obj;
        json_spirit::Object item;
        item.push_back( Pair("id", base_id) );
        item.push_back( Pair("level", level) );
        item.push_back( Pair("type", ITEM_TYPE_BAOSHI) );
        obj.push_back( Pair("cmd", "notifyGet") );
        obj.push_back( Pair("item", item) );
        obj.push_back( Pair("s", 200) );
        m_chardata.sendObj(obj);
        return oldcount - count;
    }
    else
    {
        for (int i = 1; i <= m_size; ++i)
        {
            //只算在背包里面的
            if (m_bagslot[i-1].get())
            {
                boost::shared_ptr<item_base> pp = m_bagslot[i-1];
                if (pp->getType() == ITEM_TYPE_BAOSHI)
                {
                    Baoshi* pb = dynamic_cast<Baoshi*>(pp.get());
                    if (pb->getSubType() == base_id && pb->getLevel() == level)
                    {
                        if (pp->getCount() + count >= 0)
                        {
                            pp->addCount(count);
                            if (pp->getCount() == 0)
                            {
                                removeItem(i);
                            }
                            pb->Save();
                            return oldcount;
                        }
                        else
                        {
                            count += pp->getCount();
                            pp->Clear();
                            removeItem(i);
                            pb->Save();
                        }
                    }
                }
            }
        }
        return oldcount - count;
    }
}

HeroBag::HeroBag(CharHeroData& c, size_t s)
:m_herodata(c)
,Bag_base(BAG_TYPE_HERO, s)
{
}

CharHeroData* HeroBag::getHero() const
{
    return &m_herodata;
}

void HeroBag::loadBag()
{
    Query q(GetDb());
    //加载英雄身上的装备
    q.get_result("select id,base_id,hid,level,quality,bagSize,slot from char_equipment where hid=" + LEX_CAST_STR(m_herodata.m_id)+" and cid=" + LEX_CAST_STR(m_herodata.m_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int baseid = q.getval();
        int hid = q.getval();
        int level = q.getval();
        int quality = q.getval();
        int bagSize = q.getval();
        boost::shared_ptr<baseEquipment> pe = GeneralDataMgr::getInstance()->GetBaseEquipment(baseid);
        if (!pe.get())
        {
            ERR();
            INFO("error base_equipt base_id=" << baseid);
            continue;
        }
        boost::shared_ptr<item_base> e;
        Equipment* eq = new Equipment(baseid, id, m_herodata.m_cid, level,quality);
        e.reset(eq);
        uint8_t slot = q.getval();
        if (m_bagslot[slot-1].get())
        {
            m_herodata.m_belong_to.m_charData.m_bag.addItem(e);
            e->Save();
        }
        else
        {
            addItem(slot, e);
        }
        eq->m_bag.setSize(bagSize);
        eq->m_bag.loadBag();
    }
    q.free_result();
    return;
}

EquipmentBag::EquipmentBag(Equipment& c, size_t s)
:m_equiptdata(c)
,Bag_base(BAG_TYPE_EQUIPTMENT, s)
{
}

Equipment* EquipmentBag::getEquipment() const
{
    return &m_equiptdata;
}

void EquipmentBag::loadBag()
{
    Query q(GetDb());
    q.get_result("select id,base_id,slot,level,nums from char_baoshi where eid=" + LEX_CAST_STR(m_equiptdata.getId()) + " and cid=" + LEX_CAST_STR(m_equiptdata.m_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int base_id = q.getval();
        int slot = q.getval();
        int level = q.getval();
        int nums = q.getval();

        boost::shared_ptr<baseBaoshi> pb = GeneralDataMgr::getInstance()->GetBaseBaoshi(base_id);
        if (!pb.get())
        {
            ERR();
            INFO("error base_baoshi base_id=" << base_id);
            continue;
        }
        boost::shared_ptr<item_base> e;
        Baoshi* eq = new Baoshi(id, *(pb.get()), level, nums);
        e.reset(eq);
        if (m_bagslot[slot-1].get())
        {
            CharData* pc = m_equiptdata.getChar();
            if (pc)
            {
                pc->m_bag.addItem(e);
                e->Save();
            }
        }
        else
        {
            addItem(slot, e);
        }
    }
    q.free_result();
    return;
}

