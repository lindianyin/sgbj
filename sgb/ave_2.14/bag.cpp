
#include "bag.h"
#include "data.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "equipt_upgrade.h"
#include "singleton.h"
#include "libao.h"
#include "new_event.h"
#include "statistics.h"

#define INFO(x) cout<<x
extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

const std::string strNotifyBagFull = "{\"cmd\":\"bag_full\",\"state\":$S,\"s\":200}";

inline bool canMerge(boost::shared_ptr<iItem>& a, boost::shared_ptr<iItem>& b)
{
    if (a.get() && a->getType() == iItem_type_baoshi)
    {
        if (b.get() && iItem_type_baoshi == b->getType() && a->getSubtype() == b->getSubtype())
        {
            newBaoshi* pa = dynamic_cast<newBaoshi*>(a.get());
            newBaoshi* pb = dynamic_cast<newBaoshi*>(b.get());
            return pa->level() == pb->level();
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
                && a->getSubtype() == b->getSubtype()
                && (a->maxCount() == 0
                    || a->maxCount() > a->getCount()
                    || b->maxCount() > b->getCount()) );
    }
}

//物品比较 1 a比b优先 0a和b相同 -1b比a优先
int8_t itemCompare(iItem& a, iItem& b)
{
    if (a.getType() == b.getType())
    {
        switch (a.getType())
        {
            case iItem_type_gem:
                {
                    Gem* pa = dynamic_cast<Gem*>(&a);
                    Gem* pb = dynamic_cast<Gem*>(&b);
                    //同是材料
                    if (pa->getUsage() == pb->getUsage() && pa->getUsage() == ITEM_USAGE_EQUIPMENT_METRIAL)
                    {
                        //先判断品质
                        if (pa->getQuality() > pb->getQuality())
                        {
                            return 1;
                        }
                        else if (pa->getQuality() < pb->getQuality())
                        {
                            return -1;
                        }
                    }
                    if (a.getSubtype() == b.getSubtype())
                    {
                        return 0;
                    }
                    else if (a.getSubtype() > b.getSubtype())
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                }
                break;
            case iItem_type_equipment:
                {
                    EquipmentData* pa = dynamic_cast<EquipmentData*>(&a);
                    EquipmentData* pb = dynamic_cast<EquipmentData*>(&b);
                    //先判断品质
                    if (pa->quality > pb->quality)
                    {
                        return 1;
                    }
                    else if (pa->quality < pb->quality)
                    {
                        return -1;
                    }
                    else
                    {
                        if (pa->qLevel > pb->qLevel)
                        {
                            return 1;
                        }
                        else if (pa->qLevel < pb->qLevel)
                        {
                            return -1;
                        }
                        else
                        {
                            if (pa->baseid < pb->baseid)
                            {
                                return -1;
                            }
                            else if (pa->baseid > pb->baseid)
                            {
                                return 1;
                            }
                            else if (pa->id < pb->id)
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
            case iItem_type_baoshi:
                {
                    newBaoshi* pa = dynamic_cast<newBaoshi*>(&a);
                    newBaoshi* pb = dynamic_cast<newBaoshi*>(&b);
                    if (pa->m_base.type < pb->m_base.type)
                    {
                        return 1;
                    }
                    else if (pa->m_base.type > pb->m_base.type)
                    {
                        return -1;
                    }
                    else
                    {
                        if (pa->level() > pb->level())
                        {
                            return 1;
                        }
                        else if (pa->level() < pb->level())
                        {
                            return -1;
                        }
                        return 0;
                    }
                }
                break;
        }
    }
    else
    {
        if (a.getType() == iItem_type_equipment)
        {
            return 1;
        }
        else if (b.getType() == iItem_type_equipment)
        {
            return -1;
        }
        else if (a.getType() == iItem_type_gem)
        {
            return 1;
        }
        else if (b.getType() == iItem_type_gem)
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

iItem::iItem(uint16_t type, uint16_t subType, uint32_t id, int count)
{
    m_guid.m_type = type;
    m_guid.m_subType = subType;
    m_guid.m_guid = id;
    m_count = count;
    m_container = NULL;
    m_slot = 0;
    m_owner = 0;
    m_deleteTime = 0;
    m_invalidTime = 0;
    m_state = 0;
    m_changed = false;
}

int32_t iItem::addCount(int32_t a)
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

boost::shared_ptr<CharGeneralData> iItem::getGeneral() const
{
    if (m_container)
    {
        return m_container->gd;
    }
    boost::shared_ptr<CharGeneralData> gd;
    return gd;
}

CharData* iItem::getChar() const
{
    if (m_container)
    {
        return &m_container->m_cdata;
    }
    return NULL;
}

void iItem::setDeleteTime(time_t dt)
{
    if (dt == 0)
    {
        m_deleteTime = time(NULL) + 1800;
    }
    else
    {
        m_deleteTime = dt;
    }
    m_changed = true;
    m_state = 1;
}

void iItem::setInvalidTime(time_t vt)
{
    m_invalidTime = vt;
    m_changed = true;
}

Gem::Gem(uint16_t type, uint16_t subtype, uint32_t id, int32_t count)
:iItem(type, subtype, id, count)
{
    m_base = GeneralDataMgr::getInstance()->GetBaseTreasure(subtype);
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

std::string Gem::name() const
{
    if (m_base.get())
    {
        return m_base->name;
    }
    return "a gem";
}

std::string Gem::memo() const
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

int Gem::getUsage() const
{
    if (m_base.get())
    {
        return m_base->usage;
    }
    return 0;
}

int Gem::getValue() const
{
    if (m_base.get())
    {
        return m_base->value;
    }
    return 0;
}

int Gem::getBaseInvalidTime() const
{
    if (m_base.get())
    {
        return m_base->invalidTime;
    }
    return 0;
}

uint16_t Gem::getSpic() const
{
    if (m_base.get())
    {
        return m_base->spic;
    }
    return getSubtype();
}

#ifdef QQ_PLAT
int Gem::getGoldCost() const
{
    if (m_base.get())
    {
        return m_base->gold_to_buy;
    }
    return 0;
}
#endif

void Gem::Save()
{
    if (m_changed)
    {
        int32_t c = getCount();
        if (c > 0)
        {
            //保存数据库
            InsertSaveDb("update char_treasures set nums=" + LEX_CAST_STR(c) +
                        ",slot=" + LEX_CAST_STR((int)getSlot())
                        + ",state=" + LEX_CAST_STR((int)m_state)
                        + ",deleteTime=" + LEX_CAST_STR(m_deleteTime)
                        + ",invalidTime=" + LEX_CAST_STR(m_invalidTime)
                        + " where id=" + LEX_CAST_STR(getId()));
        }    
        else
        {
            //保存数据库
            InsertSaveDb("delete from char_treasures where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

EquipmentData::EquipmentData(uint16_t subtype, int id_)
:iItem((uint16_t)iItem_type_equipment, subtype, id_, 1)
{
    value = 0;
    value2 = 0;
    addValue = 0;
    addValue2 = 0;
    type = subtype;
    id = id_;
    baseEq = GeneralDataMgr::getInstance()->GetBaseEquipment(subtype);
}

int32_t EquipmentData::sellPrice() const
{
    return price;
}

std::string EquipmentData::name() const
{
    if (baseEq.get())
    {
        return baseEq->name;
    }
    return "a equipment";
}

std::string EquipmentData::memo() const
{
    if (baseEq.get())
    {
        return baseEq->desc;
    }
    return "this is a equipment";
}

void EquipmentData::Save()
{
    if (m_changed)
    {
        //保存到数据库
        InsertSaveDb("update char_equipment set qLevel=" + LEX_CAST_STR(qLevel)
                + ",orgAttr="+ LEX_CAST_STR(value)
                + ",addAttr="+ LEX_CAST_STR(addValue)
                + ",orgAttr2="+ LEX_CAST_STR(value2)
                + ",addAttr2="+ LEX_CAST_STR(addValue2)
                + ",state="+ LEX_CAST_STR((int)m_state)
                + ",base_id="+ LEX_CAST_STR(baseid)
                + ",slot="+ LEX_CAST_STR((int)getSlot())
                + ",gid="+ LEX_CAST_STR(getGeneral().get() ? getGeneral().get()->m_id : 0)
                + ",deleteTime="+ LEX_CAST_STR(m_deleteTime)
                + " where cid="+ LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id));
        m_changed = false;
    }
}

Bag::Bag(CharData& c)
:iItem((uint16_t)iItem_type_bag, 1, 0, 1)
,m_cdata(c)
,m_size(BAG_DEFAULT_SIZE)
{
    m_sorted = false;
    m_slot_used = 0;
}

Bag::Bag(CharData& c, size_t s)
:iItem((uint16_t)iItem_type_bag, 1, 0, 1)
,m_cdata(c)
,m_size(s > MAX_BAG_SIZE ? MAX_BAG_SIZE : s)
{
    m_slot_used = 0;
    m_sorted = false;
}

void Bag::swapItem(uint8_t slot1, uint8_t slot2)
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
            boost::shared_ptr<iItem> itm1 = m_bagslot[slot1-1];
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

uint8_t Bag::addItem(uint8_t slot, boost::shared_ptr<iItem>& itm, bool notify)
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
            //通知背包状态
            if (notify)
            {
                std::string msg = strNotifyBagFull;
                str_replace(msg, "$S", (isFull() ? "true" : "false"));
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_cdata.m_name);
                if (account.get())
                {
                    account->Send(msg);
                }
            }
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

uint8_t Bag::addItem(boost::shared_ptr<iItem>& itm, bool notify)
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

boost::shared_ptr<iItem> Bag::removeItem(uint8_t slot, bool notify)
{
    boost::shared_ptr<iItem> itm;
    if (slot > 0 && slot <= m_size)
    {
        if (m_bagslot[slot-1].get())
        {
            m_bagslot[slot-1]->clearContainer();
            itm = m_bagslot[slot-1];
            m_bagslot[slot-1].reset();
            --m_slot_used;
            m_sorted = false;
            //通知背包状态
            if (notify)
            {
                std::string msg = strNotifyBagFull;
                str_replace(msg, "$S", (isFull() ? "true" : "false"));
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_cdata.m_name);
                if (account.get())
                {
                    account->Send(msg);
                }
            }
            return itm;
        }
    }
    return itm;
}

boost::shared_ptr<iItem> Bag::getItem(uint8_t slot)
{
    if (slot > 0 && slot <= m_size)
    {
        return m_bagslot[slot-1];
    }
    boost::shared_ptr<iItem> itm;
    return itm;
}

int32_t Bag::getCount(uint16_t type, uint16_t subType)
{
    int32_t count = 0;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == type && m_bagslot[i]->getSubtype() == subType)
        {
            count += m_bagslot[i]->getCount();
        }
    }
    return count;
}

int32_t Bag::addGem(uint16_t stype, int32_t count, int& err_code, bool sell_if_full)
{
    int cid = m_cdata.m_id;
    if (count == 0 || stype == 0)
    {
        return 0;
    }
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(stype);
    if (!bt.get())
    {
        return 0;
    }
    if (bt->currency)
    {
        GemCurrency& c = m_currencys[stype];
        if ((c.count + count) >= 0)
        {
            int tmp = c.count;
            c.count += count;
            //军粮增加有上限
            if (stype == treasure_type_supply)
            {
                int need_supply = 0, max_supply = 0;
                m_cdata.GetMaxSupply(need_supply,max_supply);
                if (c.count >= max_supply)
                {
                    c.count = max_supply;
                    //提示
                    json_spirit::Object robj;
                    robj.push_back( Pair("cmd", "message"));
                    robj.push_back( Pair("s", 200));
                    robj.push_back( Pair("msg", getErrMsg(HC_ERROR_SUPPLY_MAX)));
                    m_cdata.sendObj(robj);
                    
                    //通知客户端
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("stype", 2) );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_supply) );
                    m_cdata.sendObj(obj);
                }
                if (tmp < need_supply && c.count >= need_supply)
                {
                    //通知客户端
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "notify") );
                    obj.push_back( Pair("stype", 1) );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("type", notify_msg_supply) );
                    m_cdata.sendObj(obj);
                }
            }
            if (c.id == 0)
            {
                c.type = stype;
                c.id = GeneralDataMgr::getInstance()->newGemId();
                //保存数据库
                InsertSaveDb("insert into char_treasures (id,cid,tid,nums,slot) value ("
                    + LEX_CAST_STR(c.id) + ","
                    + LEX_CAST_STR(cid) + ","
                    + LEX_CAST_STR(stype) + ","
                    + LEX_CAST_STR(c.count) + ",255)");
            }
            else
            {
                //保存数据库
                InsertSaveDb("update char_treasures set nums=" + LEX_CAST_STR(c.count)+ " where id=" + LEX_CAST_STR(c.id));
            }
            //秘法队列更新
            if (stype == treasure_type_gongxun)
            {
                m_cdata.updateUpgradeWeaponCDList();
            }
            return c.count;
        }
        else
        {
            return -1;
        }
    }
    if (count > 0)
    {
        m_sorted = false;
        int32_t max_c = bt->max_size;
        if (max_c > 1 || 0 == max_c)
        {
            for (size_t i = 0; i < m_size && count > 0; ++i)
            {
                iItem* p = m_bagslot[i].get();
                if (p && p->getType() == iItem_type_gem && p->getSubtype() == stype)
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
                    InsertSaveDb("update char_treasures set nums=" + LEX_CAST_STR(p->getCount())+ " where id=" + LEX_CAST_STR(p->getId()));
                    p->unsetChanged();
                }
            }
        }
        if (count > 0)
        {
            for (size_t i = 0; i < m_size && count > 0; ++i)
            {
                iItem* p = m_bagslot[i].get();
                if (!p)
                {
                    //增加道具
                    boost::shared_ptr<iItem> t;
                    uint32_t id = GeneralDataMgr::getInstance()->newGemId();
                    if (0 == max_c || count <= max_c)
                    {
                        t.reset(dynamic_cast<iItem*>(new Gem((uint16_t)iItem_type_gem, stype, id, count)));
                        count = 0;
                    }
                    else
                    {
                        t.reset(dynamic_cast<iItem*>(new Gem((uint16_t)iItem_type_gem, stype, id, max_c)));
                        count -= max_c;
                    }
                    Gem* pg = dynamic_cast<Gem*>(t.get());
                    if (pg->getBaseInvalidTime())
                    {
                        t->setInvalidTime(pg->getBaseInvalidTime()*60 + time(NULL));
                    }
                    addItem(i + 1, t);
                    
                    //保存数据库
                    InsertSaveDb("insert into char_treasures (id,cid,tid,nums,slot,invalidTime) value ("
                        + LEX_CAST_STR(id) + ","
                        + LEX_CAST_STR(cid) + ","
                        + LEX_CAST_STR(stype) + ","
                        + LEX_CAST_STR(t->getCount()) + ","
                        + LEX_CAST_STR(i+1) + ","
                        + LEX_CAST_STR(t->getInvalidTime()) + ")");
                    t->unsetChanged();
                }
            }
            //放到回购中
            while (sell_if_full && count > 0)
            {
                //增加道具
                boost::shared_ptr<iItem> t;
                uint32_t id = GeneralDataMgr::getInstance()->newGemId();
                if (0 == max_c || count <= max_c)
                {
                    t.reset(dynamic_cast<iItem*>(new Gem((uint16_t)iItem_type_gem, stype, id, count)));
                    count = 0;
                }
                else
                {
                    t.reset(dynamic_cast<iItem*>(new Gem((uint16_t)iItem_type_gem, stype, id, max_c)));
                    count -= max_c;
                }
                t->setDeleteTime();
                Gem* pg = dynamic_cast<Gem*>(t.get());
                if (pg->getBaseInvalidTime())
                {
                    t->setInvalidTime(pg->getBaseInvalidTime()*60 + time(NULL));
                }
                m_cdata.m_selled_bag.add(t);
                err_code = HC_ERROR_BACKPACK_FULL_GET_EQUIPT;

                //保存数据库
                InsertSaveDb("insert into char_treasures (id,cid,tid,nums,slot,state,deleteTime,invalidTime) value ("
                    + LEX_CAST_STR(id) + ","
                    + LEX_CAST_STR(cid) + ","
                    + LEX_CAST_STR(stype) + ","    
                    + LEX_CAST_STR(t->getCount()) + ",0,1,"
                    + LEX_CAST_STR(t->getDeleteTime()) + ","
                    + LEX_CAST_STR(t->getInvalidTime()) + ")");
                t->unsetChanged();
            }
        }
        //通知客户端
        json_spirit::Object obj;
        json_spirit::Object item;
        item.push_back( Pair("spic", bt->spic) );
        item.push_back( Pair("type", item_type_treasure) );
        obj.push_back( Pair("cmd", "notify") );
        obj.push_back( Pair("item", item) );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("type", notify_msg_new_get) );
        m_cdata.sendObj(obj);
        if (bt->b_used_for_task)
        {
            int32_t xcount = getCount((uint16_t) iItem_type_gem, stype);
            m_cdata.m_trunk_tasks.updateTask(task_get_gem, stype, xcount);
            return xcount;
        }
        else
        {
            return getCount((uint16_t) iItem_type_gem, stype);
        }
    }
    else
    {
        int32_t cur_c = getCount((uint16_t) iItem_type_gem, stype);
        if ((cur_c + count) < 0)
        {
            return -1;
        }
        m_sorted = false;
        for (size_t i = 0; i < m_size && count < 0; ++i)
        {
            iItem* p = m_bagslot[i].get();
            if (p && p->getType() == iItem_type_gem && p->getSubtype() == stype)
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
                    InsertSaveDb("delete from char_treasures where id=" + LEX_CAST_STR(p->getId()));
                    removeItem(i+1);
                }
                else
                {
                    p->Save();
                }
            }
        }
        if (bt->b_used_for_task)
        {
            int32_t xcount = getCount((uint16_t) iItem_type_gem, stype);
            m_cdata.m_trunk_tasks.updateTask(task_get_gem, stype, xcount);
            return xcount;
        }
        else
        {
            return getCount((uint16_t) iItem_type_gem, stype);
        }
    }
}

int32_t Bag::getGemCurrency(uint16_t stype)
{
    GemCurrency& c = m_currencys[stype];
    return c.count;
}

int32_t Bag::getGemCount(uint16_t stype)
{
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(stype);
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
        return getCount((uint16_t) iItem_type_gem, stype);
    }
}

size_t Bag::addSize(size_t a)
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

bool Bag::CheckHasEquipt(int base_id)
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
            if (p->baseid == base_id)
            {
                return true;
            }
        }
    }
    return false;
}

int Bag::maxEquipLevel(int type, int& id)
{
    id = 0;
    int level = -1;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment && m_bagslot[i]->getSubtype() == type)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
            if (p->qLevel > level)
            {
                level = p->qLevel;
                id = p->id;
            }
        }
    }
    return level;
}

int Bag::getBestEquipment(int type, int maxNeedLevel, int quality, int level)
{
    //cout<<"getBestEquipment("<<type<<","<<maxNeedLevel<<","<<quality<<","<<level<<")"<<endl;
    int id = 0;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
            //cout<<"->"<<p->type<<","<<p->baseEq->needLevel<<","<<p->up_quality<<","<<p->qLevel<<endl;
            if (p->type == type
                && p->baseEq->needLevel <= maxNeedLevel
                && (p->up_quality > quality
                   || (quality == p->up_quality && p->qLevel > level)
                   )
                )
            {
                quality = p->up_quality;
                level = p->qLevel;
                id = p->id;
                //cout<<"find id = "<<id<<endl;
            }
        }
    }
    return id;
}

EquipmentData* Bag::getEquipById(int id)
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
            if (p->id == id)
            {
                return p;
            }
        }
    }
    return NULL;
}

int Bag::getMinEnhanceCost(int& eid)
{
    int minCost = 0;
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
            if (p->qLevel < m_cdata.m_level)
            {
                int org = 0;
                int cost = equipmentUpgrade::getInstance()->getUpgradeNeeds(p->quality, p->type, p->qLevel+1, org);
                if (cost > 0 && (minCost == 0 || cost < minCost))
                {
                    minCost = cost;
                    eid = p->id;
                }
            }
        }
    }
    return minCost;
}

EquipmentData* Bag::getDefaultEquip(int type, int max_level)
{
    if (0 == type)
    {
        int level = -1;
        EquipmentData* pp = NULL;
        for (size_t i = 0; i < m_size; ++i)
        {
            if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment)
            {
                EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
                if (p->qLevel < max_level)
                {
                    int org = 0;
                    if (-1 == level 
                        ||    (p->qLevel > level
                        //强化消耗的是银币
                            && equipmentUpgrade::getInstance()->getUpgradeNeeds(p->quality, p->type, 1 + p->qLevel, org) <= m_cdata.silver()))
                    {
                        pp = p;
                        level = p->qLevel;
                    }
                }
            }
        }
        return pp;
    }
    else
    {
        int level = -1;
        EquipmentData* pp = NULL;
        for (size_t i = 0; i < m_size; ++i)
        {
            if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment && type == m_bagslot[i]->getSubtype())
            {
                EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
                if (p->qLevel < max_level)
                {
                    if (-1 == level ||    p->qLevel > level)
                    {
                        pp = p;
                        level = p->qLevel;
                    }
                }
            }
        }
        return pp;
    }
}

boost::shared_ptr<iItem> Bag::getEquipItemById(int id)
{
    for (size_t i = 0; i < m_size; ++i)
    {
        if (m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>(m_bagslot[i].get());
            if (p->id == id)
            {
                return m_bagslot[i];
            }
        }
    }
    boost::shared_ptr<iItem> nil;
    return nil;
}

newBaoshi* Bag::getBaoshiCanMerge(int type, int level, int count)
{
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi)
            {
                newBaoshi* pb = dynamic_cast<newBaoshi*>(pp);
                if (pb->m_base.type == type && level == pb->level())
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

//返回实际增加
int Bag::addBaoshi(int type, int level, int count)
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
                iItem* pp = m_bagslot[i-1].get();
                if (pp->getType() == iItem_type_baoshi)
                {
                    newBaoshi* pb = dynamic_cast<newBaoshi*>(pp);
                    if (pb->m_base.type == type && level == pb->level())
                    {
                        if (pp->getCount() + count <= pp->maxCount())
                        {
                            pp->addCount(count);
                            pb->Save();
                            //通知客户端
                            json_spirit::Object obj;
                            json_spirit::Object item;
                            item.push_back( Pair("type", item_type_baoshi) );
                            item.push_back( Pair("id", type) );
                            item.push_back( Pair("fac", level) );
                            item.push_back( Pair("level", level) );
                            obj.push_back( Pair("item", item) );
                            obj.push_back( Pair("cmd", "notify") );
                            obj.push_back( Pair("s", 200) );
                            obj.push_back( Pair("type", notify_msg_new_get) );
                            m_cdata.sendObj(obj);
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

            boost::shared_ptr<iItem> bs = Singleton<newBaoshiMgr>::Instance().cloneBaoshi(type, level, bscount);
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
                cout<<"!!!!!!!!!!!!!! baoshi type"<<type<<",level:"<<level<<","<<bscount<<endl;
            }
        }
        //通知客户端
        json_spirit::Object obj;
        json_spirit::Object item;
        item.push_back( Pair("type", item_type_baoshi) );
        item.push_back( Pair("id", type) );
        item.push_back( Pair("fac", level) );
        item.push_back( Pair("level", level) );
        obj.push_back( Pair("item", item) );
        obj.push_back( Pair("cmd", "notify") );
        obj.push_back( Pair("s", 200) );
        obj.push_back( Pair("type", notify_msg_new_get) );
        m_cdata.sendObj(obj);
        if (oldcount > count)
        {
            //宝石活动
            Singleton<new_event_mgr>::Instance().addBaoshi(m_cdata.m_id, level);
        }
        return oldcount - count;
    }
    else
    {
        for (int i = 1; i <= m_size; ++i)
        {
            //只算在背包里面的
            if (m_bagslot[i-1].get())
            {
                boost::shared_ptr<iItem> pp = m_bagslot[i-1];
                if (pp->getType() == iItem_type_baoshi)
                {
                    newBaoshi* pb = dynamic_cast<newBaoshi*>(pp.get());
                    if (pb->m_base.type == type && level == pb->level())
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

int Bag::getBestBagEquipments(int level, int type, json_spirit::Object& robj)
{
    EquipmentData* eq = NULL;
    int best_level = 0, best_quality = 0;
    json_spirit::Object best_eq;
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的同类型最高等级
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_equipment)
            {
                EquipmentData* p = dynamic_cast<EquipmentData*>(pp);
                if (p->baseEq.get() && p->baseEq->needLevel <= m_cdata.m_level && p->type == type && (p->qLevel > best_level || (p->qLevel == best_level && p->quality > best_quality)))
                {
                    eq = p;
                    best_level = p->qLevel;
                    best_quality = p->quality;
                }
            }
        }
    }
    //boost::shared_ptr<iItem> eqq = getEquipItemById(chose_eid);
    if (!eq)
    {
        //没获取到可推荐的则显示基础的
        boost::shared_ptr<baseEquipment> beq = GeneralDataMgr::getInstance()->GetBaseEquipment(type);
        robj.push_back( Pair("have_best", false) );
        robj.push_back( Pair("type", beq->type) );
        best_eq.push_back( Pair("itype", iItem_type_equipment) );
        best_eq.push_back( Pair("id", beq->baseid) );
        best_eq.push_back( Pair("quality", beq->quality) );
        best_eq.push_back( Pair("name", beq->name) );
        best_eq.push_back( Pair("type", beq->type) );
        best_eq.push_back( Pair("spic", beq->baseid) );
        //robj.push_back( Pair("level", 1) );
        best_eq.push_back( Pair("addNums", beq->baseValue) );
        if (beq->baseValue2 > 0)
        {
            best_eq.push_back( Pair("addNums2", beq->baseValue2) );
        }
        robj.push_back( Pair("best_eq", best_eq) );
        return HC_SUCCESS;
    }
    //EquipmentData* eq = dynamic_cast<EquipmentData*>(eqq.get());
    robj.push_back( Pair("have_best", true) );
    robj.push_back( Pair("type", eq->type) );
    best_eq.push_back( Pair("itype", iItem_type_equipment) );
    best_eq.push_back( Pair("id", eq->id) );
    best_eq.push_back( Pair("quality", eq->quality) );
    best_eq.push_back( Pair("name", eq->name()) );
    best_eq.push_back( Pair("type", eq->type) );
    best_eq.push_back( Pair("spic", eq->getSpic()) );
    best_eq.push_back( Pair("level", eq->qLevel) );
    int cur = eq->getvalue();
    int add2 = 0;
    int add = equipmentUpgrade::getInstance()->getUpgradeValue(eq->up_quality, type, eq->qLevel + 1, add2);
    best_eq.push_back( Pair("addNums", cur) );
    best_eq.push_back( Pair("nextNums", cur + add) );
    if (add2 > 0)
    {
        int cur2 = eq->getvalue2();
        best_eq.push_back( Pair("addNums2", cur2) );
        best_eq.push_back( Pair("nextNums2", cur2 + add2) );
    }
    robj.push_back( Pair("best_eq", best_eq) );
    return HC_SUCCESS;
}

int Bag::showBagEquipments(json_spirit::Object& robj)
{
    json_spirit::Array elists;
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_equipment)
            {
                EquipmentData* p = dynamic_cast<EquipmentData*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", iItem_type_equipment) );
                obj.push_back( Pair("id", p->id) );
                obj.push_back( Pair("quality", p->quality) );
                obj.push_back( Pair("name", p->name()) );
                obj.push_back( Pair("type", p->type) );
                obj.push_back( Pair("spic", p->getSpic()) );
                obj.push_back( Pair("level", p->qLevel) );
                obj.push_back( Pair("slot", i) );
                elists.push_back(obj);
            }            
        }
    }
    robj.push_back( Pair("equipList", elists) );
    robj.push_back( Pair("type", iItem_type_equipment) );
    return HC_SUCCESS;
}

int Bag::showBagBaoshis(json_spirit::Object& robj)
{
    json_spirit::Array elists;
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi)
            {
                newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", iItem_type_baoshi) );
                obj.push_back( Pair("id", (int)pp->getId()) );
                obj.push_back( Pair("quality", p->getQuality()) );
                obj.push_back( Pair("name", p->name()) );
                obj.push_back( Pair("tname", p->name()) );
                obj.push_back( Pair("type", p->m_base.type) );
                obj.push_back( Pair("spic", p->getSpic()) );
                obj.push_back( Pair("level", p->level()) );
                obj.push_back( Pair("nums", p->getCount()) );

                obj.push_back( Pair("slot", i) );
                elists.push_back(obj);
            }
        }
    }
    robj.push_back( Pair("list", elists) );
    robj.push_back( Pair("type", iItem_type_baoshi) );
    return HC_SUCCESS;
}

int Bag::showGeneralBaoshis(json_spirit::Object& robj)
{
    json_spirit::Array elists;
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_baoshi)
            {
                newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", iItem_type_baoshi) );
                obj.push_back( Pair("id", (int)pp->getId()) );
                obj.push_back( Pair("quality", p->getQuality()) );
                obj.push_back( Pair("name", p->name()) );
                obj.push_back( Pair("tname", p->name()) );
                obj.push_back( Pair("type", p->m_base.type) );
                obj.push_back( Pair("spic", p->getSpic()) );
                obj.push_back( Pair("level", p->level()) );

                obj.push_back( Pair("slot", i) );
                elists.push_back(obj);
            }
        }
        //else
        //{
        //    json_spirit::Object obj;
        //    obj.push_back( Pair("slot", i) );
        //    elists.push_back(obj);
        //}
    }
    robj.push_back( Pair("list", elists) );
    return HC_SUCCESS;
}

int Bag::baoshiAttrList(json_spirit::Array& alist)
{
    for (int i = 1; i <= m_size; ++i)
    {
        //只算在背包里面的
        if (m_bagslot[i-1].get() && m_bagslot[i-1]->getType() == iItem_type_baoshi)
        {
            newBaoshi* p = dynamic_cast<newBaoshi*>(m_bagslot[i-1].get());
            alist.push_back(p->getObj());
        }
    }
    return 0;
}

bool Bag::canXiangqian(uint8_t slot, int type)
{
    for (int i = 0; i < m_size; ++i)
    {
        if ((i+1) != slot && m_bagslot[i].get() && m_bagslot[i]->getType() == iItem_type_baoshi && m_bagslot[i]->getSubtype() == type)
        {
            return false;
        }
    }
    return true;
}

int Bag::addLibao(int id)
{
    return libao_mgr::getInstance()->addLibao(&m_cdata,id);
}

int Bag::showBag(int page, int nums_per_page, json_spirit::Object& robj)
{
    if (page < 1)
    {
        page = 1;
    }
    int maxpage = m_size / 45 + 1;
    json_spirit::Array elists;
    json_spirit::Array ilists;

    nums_per_page = 45;
    if (maxpage > MAX_BAG_SIZE/nums_per_page)
    {
        maxpage = MAX_BAG_SIZE/nums_per_page;
    }

    int first_nums = nums_per_page * (page - 1) + 1;
    int last_nums = nums_per_page * page;

    if (last_nums > MAX_BAG_SIZE)
    {
        first_nums = 1;
        last_nums = 45;
        nums_per_page = 45;
        page = 1;
    }

    for (int i = first_nums; i <= last_nums; ++i)
    {
        if (i <= m_size && m_bagslot[i-1].get())
        //只算在背包里面的
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getInvalidTime() != 0 && pp->getInvalidTime() <= time(NULL))
            {
                boost::shared_ptr<iItem> itm = removeItem(i);
                if (itm.get())
                {
                    itm->Clear();
                    itm->Save();
                }
                continue;
            }
            if (pp->getType() == iItem_type_equipment)
            {
                EquipmentData* p = dynamic_cast<EquipmentData*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", iItem_type_equipment) );
                obj.push_back( Pair("id", p->id) );
                obj.push_back( Pair("spic", p->getSpic()) );
                obj.push_back( Pair("type", p->type) );
                obj.push_back( Pair("quality", p->quality) );
                obj.push_back( Pair("level", p->qLevel) );
                obj.push_back( Pair("name", p->name()) );
                obj.push_back( Pair("price", p->sellPrice()) );
                obj.push_back( Pair("slot", i) );
                elists.push_back(obj);
            }
            else if (pp->getType() == iItem_type_gem)
            {
                Gem* p = dynamic_cast<Gem*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", iItem_type_gem) );
                obj.push_back( Pair("id", p->getSubtype()) );
                obj.push_back( Pair("spic", p->getSpic()) );
                if (p->maxCount()  > 1 || p->maxCount() == 0)
                {
                    obj.push_back( Pair("nums", p->getCount()) );
                }
                obj.push_back( Pair("quality", p->getQuality()) );
                obj.push_back( Pair("name", p->name()) );
                obj.push_back( Pair("price", p->sellPrice()) );
                obj.push_back( Pair("level", 1) );
                obj.push_back( Pair("use", p->getUsage()) );
                obj.push_back( Pair("slot", i) );
                if (pp->getInvalidTime() != 0)
                    obj.push_back( Pair("invalidTime", (int)(pp->getInvalidTime())) );
                elists.push_back(obj);
            }
            else if (pp->getType() == iItem_type_libao)
            {
                libao* p = dynamic_cast<libao*>(pp);
                json_spirit::Object obj;
                obj.push_back( Pair("itype", iItem_type_libao) );
                obj.push_back( Pair("type", p->getSubtype()) );
                obj.push_back( Pair("id", (uint64_t)p->getId()) );
                obj.push_back( Pair("spic", p->getSpic()) );
                obj.push_back( Pair("nums", p->getCount()) );
                obj.push_back( Pair("quality", p->getQuality()) );
                obj.push_back( Pair("name", p->name()) );
                obj.push_back( Pair("price", p->sellPrice()) );
                obj.push_back( Pair("level", 1) );
                obj.push_back( Pair("slot", i) );
                elists.push_back(obj);
            }
            else
            {
                json_spirit::Object obj;
                obj.push_back( Pair("itype", pp->getType()) );
                obj.push_back( Pair("type", pp->getSubtype()) );
                obj.push_back( Pair("id", (uint64_t)pp->getId()) );
                obj.push_back( Pair("spic", (uint64_t)pp->getSpic()) );
                obj.push_back( Pair("nums", pp->getCount()) );
                obj.push_back( Pair("quality", pp->getQuality()) );
                obj.push_back( Pair("name", pp->name()) );
                obj.push_back( Pair("price", pp->sellPrice()) );
                if (pp->getType() == iItem_type_baoshi)
                {
                    newBaoshi* p = dynamic_cast<newBaoshi*>(pp);
                    obj.push_back( Pair("level", p->level()) );
                }
                else
                {
                    obj.push_back( Pair("level", 1) );
                }
                obj.push_back( Pair("slot", i) );
                elists.push_back(obj);
            }
        }
    }

    robj.push_back( Pair("equipList", elists) );
    robj.push_back( Pair("usedSize", m_slot_used) );
    robj.push_back( Pair("bagSize", m_size) );
    robj.push_back( Pair("buyedSize", m_size - BAG_DEFAULT_SIZE) );
    robj.push_back( Pair("bag_key", getGemCount(treasure_type_bag_key)) );

    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );
    return HC_SUCCESS;
}

bool Bag::isFull()
{
    return m_slot_used >= m_size;
}

//排序插入
void Bag::sortInsert(boost::shared_ptr<iItem>& itm)
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
                    if (itm->getType() == iItem_type_gem)
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
                    else if (itm->getType() == iItem_type_baoshi)
                    {
                        //宝石也合并
                        int canAdd = m_bagslot[i]->maxCount() - m_bagslot[i]->getCount();
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

//背包整理
void Bag::sortBag()
{
    if (!m_sorted)
    {
        m_sorted = true;
        boost::shared_ptr<iItem> bagslot[MAX_BAG_SIZE];
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
        //通知背包状态
        std::string msg = strNotifyBagFull;
        str_replace(msg, "$S", (isFull() ? "true" : "false"));
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_cdata.m_name);
        if (account.get())
        {
            account->Send(msg);
        }
    }
}

void Bag::loadCharBag(int cid)
{
    //INFO("*************** CharEquipments::load *********************");
    Query q(GetDb());
    q.get_result("select ce.id,ce.base_id,be.type,ce.qLevel,ce.orgAttr,ce.orgAttr2,ce.addAttr,ce.addAttr2,be.quality,be.up_quality,ce.slot from char_equipment as ce left join base_equipment as be on ce.base_id=be.id where ce.gid=0 and ce.state=0 and ce.cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int baseid = q.getval();
        uint16_t type = q.getval();
        boost::shared_ptr<iItem> e;
        EquipmentData* eq = new EquipmentData(baseid, id);
        e.reset(eq);

        eq->cid = cid;
        eq->id = id;
        eq->baseid = baseid;
        eq->type = type;
        eq->qLevel = q.getval();
        eq->value = q.getval();
        eq->value2 = q.getval();
        eq->addValue = q.getval();
        eq->addValue2 = q.getval();
        eq->quality = q.getval();
        eq->up_quality = q.getval();
        uint8_t slot = q.getval();
        eq->price = eq->quality > 0 ? 50*eq->quality*(eq->qLevel+1)*(eq->qLevel+20) : 50*(eq->qLevel+1)*(eq->qLevel+20);
        if (eq->type < 1 || eq->type > equip_slot_max)
        {
            continue;
        }

        //位置上已有东西
        if (slot >= 1 && slot <= m_size)
        {
            if (getItem(slot).get())
            {
                slot = addItem(e);
                //保存
                eq->Save();
            }
            else
            {
                addItem(slot, e);
            }
        }
        else
        {
            addItem(e);
            //保存
            eq->Save();
        }
        eq->baseEq = GeneralDataMgr::getInstance()->GetBaseEquipment(eq->baseid);
    }
    q.free_result();

    //加道具
    time_t t_now = time(NULL);
    q.get_result("select tid,nums,slot,id,invalidTime from char_treasures where state=0 and cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        uint16_t tid = q.getval();
        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
        if (tr.get())
        {

            int32_t count = q.getval();
            uint8_t slot = q.getval();
            uint32_t id = q.getval();
            time_t invalidTime = q.getval();

            if (tr->currency)
            {
                GemCurrency& cc = m_currencys[tid];
                cc.id = id;
                cc.type = tid;
                cc.count = count;
            }
            else
            {
                if (invalidTime == 0 || invalidTime > t_now)
                {
                    boost::shared_ptr<iItem> t;
                    t.reset(new Gem((uint16_t)iItem_type_gem, tid, id, count));
                    t->setInvalidTime(invalidTime);
                    t->unsetChanged();
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
        else
        {
            ERR();
            INFO("error treasure id "<<tid<<",cid:"<<cid);
        }
    }
    q.free_result();
    //加礼包
    q.get_result("select libao_id,slot,id from char_libao where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        uint16_t libao_id = q.getval();
        baseLibao* p = libao_mgr::getInstance()->getBaselibao(libao_id);
        if (p)
        {
            uint8_t slot = q.getval();
            int id = q.getval();
            boost::shared_ptr<iItem> t;
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
            INFO("error libao id "<<libao_id<<",cid:"<<cid);
        }
    }
    q.free_result();
    return;
}

#ifdef QQ_PLAT
void Bag::login_to_tencent()
{
    //只算在背包里面的可购买道具
    json_spirit::Object obj;
    for (int i = 1; i <= m_size; ++i)
    {
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_gem)
            {
                Gem* p = dynamic_cast<Gem*>(pp);
                if (p->getGoldCost() > 0)
                {
                    obj.push_back( Pair(LEX_CAST_STR(p->getSubtype()), p->getCount()) );
                }
            }
        }
    }
    //玉石
    obj.push_back( Pair("9", getGemCount(treasure_type_yushi)) );
    bag_to_tencent(&m_cdata, json_spirit::write(obj));
}

void Bag::logout_to_tencent()
{
    //act_to_tencent(&m_cdata,act_bag_use,m_slot_used);
    //act_to_tencent(&m_cdata,act_bag_free,m_size-m_slot_used);
}
#endif

int Bag::getChengzhangLibaoSlot(int libao_id)
{
    for (int i = 1; i <= m_size; ++i)
    {
        if (m_bagslot[i-1].get())
        {
            iItem* pp = m_bagslot[i-1].get();
            if (pp->getType() == iItem_type_libao)
            {
                libao* pl = dynamic_cast<libao*>(pp);
                if (pl && pl->m_base.m_libao_id == libao_id && libao_mgr::getInstance()->isChengzhangLibao(pl->m_base.m_libao_id))
                {
                    return i;
                }
            }
        }
    }
    return 0;
}

selledBag::selledBag(CharData& c)
:Bag(c)
{
    
}

void selledBag::load()
{
    //INFO("*************** CharEquipments::load *********************");
    Query q(GetDb());
    q.get_result("select ce.id,ce.base_id,be.type,ce.qLevel,ce.orgAttr,ce.addAttr,ce.orgAttr2,ce.addAttr2,be.quality,be.up_quality,ce.deleteTime from char_equipment as ce left join base_equipment as be on ce.base_id=be.id where ce.deleteTime>unix_timestamp() and ce.state=1 and ce.cid=" + LEX_CAST_STR(m_cdata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int baseid = q.getval();
        uint16_t type = q.getval();
        boost::shared_ptr<iItem> e;
        EquipmentData* eq = new EquipmentData(baseid, id);
        e.reset(eq);

        eq->cid = m_cdata.m_id;
        eq->id = id;
        eq->baseid = baseid;
        eq->type = type;
        eq->qLevel = q.getval();
        eq->value = q.getval();
        eq->addValue = q.getval();
        eq->value2 = q.getval();
        eq->addValue2 = q.getval();
        eq->quality = q.getval();
        eq->up_quality = q.getval();

        eq->setDeleteTime(q.getval());

        eq->price = eq->quality > 0 ? 50*eq->quality*(eq->qLevel+1)*(eq->qLevel+20) : 50*(eq->qLevel+1)*(eq->qLevel+20);
        if (eq->type >= 1 && eq->type <= equip_slot_max)
        {
        }
        else
        {
            continue;
        }

        eq->baseEq = GeneralDataMgr::getInstance()->GetBaseEquipment(eq->baseid);

        add(e);
    }
    q.free_result();

    //加道具
    q.get_result("select tid,nums,slot,id,deleteTime from char_treasures where state=1 and deleteTime>unix_timestamp() and cid=" + LEX_CAST_STR(m_cdata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        uint16_t tid = q.getval();
        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
        if (tr.get())
        {

            int32_t count = q.getval();
            uint8_t slot = q.getval();
            uint32_t id = q.getval();

            if (tr->currency)
            {
                continue;
            }
            else
            {
                boost::shared_ptr<iItem> t;
                t.reset(new Gem((uint16_t)iItem_type_gem, tid, id, count));
                t->setDeleteTime(q.getval());
                add(t);
            }
        }
        else
        {
            ERR();
            INFO("error treasure id "<<tid<<",cid:"<<m_cdata.m_id);
        }
    }
    q.free_result();

    //INFO("*************** CharEquipments::load *********************");
    q.get_result("select id,type,gid,slot,level,nums,deleteTime from char_baoshi where state=1 and deleteTime>unix_timestamp() and cid=" + LEX_CAST_STR(m_cdata.m_id));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int type = q.getval();
        int gid = q.getval();
        int slot = q.getval();
        int level = q.getval();
        int nums = q.getval();
        //int exp = q.getval();

        baseNewBaoshi* bbs = Singleton<newBaoshiMgr>::Instance().getBaoshi(type);
        if (!bbs)
        {
            continue;
        }
        boost::shared_ptr<iItem> e;
        newBaoshi* eq = new newBaoshi(id, *bbs, level, nums);
        e.reset(eq);

        Singleton<newBaoshiMgr>::Instance().registerBaoshi(id, e);
        e->setDeleteTime(q.getval());
        add(e);
    }
    q.free_result();
}

void selledBag::add(boost::shared_ptr<iItem>& itm)
{
    if (!itm.get())// || itm->getType() != iItem_type_equipment)
    {
        return;
    }
    std::vector<boost::shared_ptr<iItem> >::iterator it = m_selled_list.begin();
    while (it != m_selled_list.end())
    {
        if ((*it)->getDeleteTime() < itm->getDeleteTime())
        {
            break;
        }
        ++it;
    }
    itm->setContainer(this);
    m_selled_list.insert(it, itm);
}

/*boost::shared_ptr<iItem> selledBag::remove(int pos)
{
    boost::shared_ptr<iItem> itm;
    if (pos >= 1 && pos <= m_selled_list.size())
    {
        itm = m_selled_list[pos-1];
        m_selled_list.erase(m_selled_list.begin() + pos - 1);
    }
    return itm;
}*/

boost::shared_ptr<iItem> selledBag::removeByEquipId(int id)
{
    boost::shared_ptr<iItem> itm;
    std::vector<boost::shared_ptr<iItem> >::iterator it = m_selled_list.begin();
    while (it != m_selled_list.end())
    {
        if ((*it).get()
            && (*it)->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>((*it).get());
            if (p->id == id)
            {
                itm = *it;
                itm->clearContainer();
                m_selled_list.erase(it);
                break;
            }
        }
        ++it;
    }
    return itm;
}

EquipmentData* selledBag::getEquipById(int id)
{
    std::vector<boost::shared_ptr<iItem> >::iterator it = m_selled_list.begin();
    while (it != m_selled_list.end())
    {
        if ((*it).get() && (*it)->getType() == iItem_type_equipment)
        {
            EquipmentData* p = dynamic_cast<EquipmentData*>((*it).get());
            if (p->id == id)
            {
                return p;
            }
        }
        ++it;
    }
    return NULL;
}

int selledBag::buyBack(int type, int id)
{
    boost::shared_ptr<iItem> itm;
    std::vector<boost::shared_ptr<iItem> >::iterator it = m_selled_list.begin();
    while (it != m_selled_list.end())
    {
        if ((*it).get()
            && (*it)->getType() == type && (*it)->getId() == id)
        {
            itm = *it;
            if (m_cdata.addSilver(-itm->sellPrice()*itm->getCount()*2, true) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_SILVER;
            }
            m_cdata.NotifyCharData();
            itm->clearContainer();
            m_cdata.m_bag.addItem(itm);
            itm->buyBack();            
            itm->Save();
            m_selled_list.erase(it);

            if (itm->getType() == item_type_treasure)
            {
                boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(itm->getSubtype());
                if (bt.get() && bt->b_used_for_task)
                {
                    int32_t xcount = m_cdata.m_bag.getCount((uint16_t) iItem_type_gem, itm->getSubtype());
                    //cout<<"update trank task "<<itm->getSubtype()<<","<<xcount<<endl;
                    m_cdata.m_trunk_tasks.updateTask(task_get_gem, itm->getSubtype(), xcount);
                }
            }
            else if (item_type_baoshi == itm->getType())
            {
                newBaoshi* pb = dynamic_cast<newBaoshi*>(itm.get());
                //宝石回购统计
                add_statistics_of_baoshi_get(m_cdata.m_id, m_cdata.m_ip_address, m_cdata.m_union_id, m_cdata.m_server_id, itm->getSubtype(), pb->level(), itm->getCount(), baoshi_sell);
            }
            return HC_SUCCESS;
        }
        ++it;
    }
    return HC_ERROR;
}

void selledBag::getList(int page, int nums_per_page, json_spirit::Object& robj)
{
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page == 0)
    {
        nums_per_page = 10;
    }
    int cur_nums = 0;
    int first_nums = nums_per_page * (page - 1)+ 1;
    int last_nums = nums_per_page * page;

    json_spirit::Array elists;
    std::vector<boost::shared_ptr<iItem> >::iterator it = m_selled_list.begin();
    while (it != m_selled_list.end())
    {
        if ((*it)->getDeleteTime() > time(NULL))
        {
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                if ((*it)->getType() == iItem_type_equipment)
                {
                    EquipmentData* p = dynamic_cast<EquipmentData*>((*it).get());
                    json_spirit::Object obj;
                    obj.push_back( Pair("id", p->id) );
                    obj.push_back( Pair("type", p->type) );
                    obj.push_back( Pair("quality", p->quality) );
                    obj.push_back( Pair("level", p->qLevel) );
                    obj.push_back( Pair("price", p->price*2) );
                    obj.push_back( Pair("addNums", p->getvalue()) );
                    if (p->getvalue2())
                    {
                        obj.push_back( Pair("addNums2", p->getvalue2()) );
                    }
                    obj.push_back( Pair("recycleTime", (int)(p->getDeleteTime() - time(NULL))) );
                    if (p->baseEq.get())
                    {
                        obj.push_back( Pair("name", p->baseEq->name) );
                        obj.push_back( Pair("spic", p->baseEq->baseid) );
                    }
                    obj.push_back( Pair("guid", (int)p->getId()) );
                    obj.push_back( Pair("itype", iItem_type_equipment) );
                    elists.push_back(obj);
                }
                else if ((*it)->getType() == iItem_type_gem)
                {
                    Gem* p = dynamic_cast<Gem*>((*it).get());
                    json_spirit::Object obj;
                    obj.push_back( Pair("itype", iItem_type_gem) );
                    obj.push_back( Pair("name", p->name()) );
                    obj.push_back( Pair("id", p->getSubtype()) );
                    obj.push_back( Pair("spic", p->getSpic()) );

                    if (p->maxCount() > 1 || 0 == p->maxCount())
                    {
                        obj.push_back( Pair("nums", p->getCount()) );
                    }
                    obj.push_back( Pair("quality", p->getQuality()) );
                    obj.push_back( Pair("level", 1) );
                    obj.push_back( Pair("price", (*it)->sellPrice()*2) );
                    obj.push_back( Pair("use", p->getUsage()) );
                    obj.push_back( Pair("guid", (int)p->getId()) );
                    obj.push_back( Pair("recycleTime", (int)(p->getDeleteTime() - time(NULL))) );
                    elists.push_back(obj);
                }
                else
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("itype", (*it)->getType()) );
                    obj.push_back( Pair("name", (*it)->name()) );
                    obj.push_back( Pair("type", (*it)->getSubtype()) );
                    obj.push_back( Pair("id", (uint64_t)(*it)->getId()) );
                    obj.push_back( Pair("spic", (uint64_t)(*it)->getSpic()) );
                    obj.push_back( Pair("nums", (*it)->getCount()) );
                    obj.push_back( Pair("quality", (*it)->getQuality()) );
                    obj.push_back( Pair("price", (*it)->sellPrice()*2) );
                    if ((*it)->getType() == iItem_type_baoshi)
                    {
                        newBaoshi* p = dynamic_cast<newBaoshi*>((*it).get());
                        obj.push_back( Pair("level", p->level()) );
                    }
                    else
                    {
                        obj.push_back( Pair("level", 1) );
                    }
                    obj.push_back( Pair("guid", (uint64_t)(*it)->getId()) );
                    obj.push_back( Pair("recycleTime", (int)((*it)->getDeleteTime() - time(NULL))) );
                    //obj.push_back( Pair("slot", i) );
                    elists.push_back(obj);
                }
            }            
            ++it;
        }
        else
        {
            //it->Clear();
            //it->Save();
            it = m_selled_list.erase(it);
        }
    }
    robj.push_back( Pair("list", elists) );
    
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
}

void swapBag(Bag& b1, uint8_t slot1, Bag& b2, uint8_t slot2)
{
    if (slot1 < 1 || slot1 > b1.size() || slot2 < 1 || slot2 > b2.size())
    {
        return;
    }
    if (&b1 == &b2)
    {
        b1.swapItem(slot1, slot2);
        return;
    }
    boost::shared_ptr<iItem> itm1 = b1.getItem(slot1);
    boost::shared_ptr<iItem> itm2 = b2.getItem(slot2);
    if (!itm1.get() && !itm2.get())
    {
        return;
    }
    if (!itm1.get())
    {
        b2.removeItem(slot2);
        b1.addItem(slot1, itm2);
        itm2->Save();
        return;
    }
    if (!itm2.get())
    {
        b1.removeItem(slot1);
        b2.addItem(slot2, itm1);
        itm1->Save();
        return;
    }
    if (canMerge(itm1, itm2))
    {
        int32_t c1 = itm1->getCount();
        int32_t c2 = itm2->getCount();
        int32_t max_c = itm1->maxCount();
        if (max_c == 0 || (c1+c2) <= max_c)
        {
            itm2->addCount(c1);
            itm2->Save();
            b1.removeItem(slot1);
            itm1->Clear();
            itm1->Save();
        }
        else
        {
            itm2->addCount(max_c - c2);
            itm1->addCount(c2 - max_c);
            itm1->Save();
            itm2->Save();
        }
    }
    else
    {
        b1.removeItem(slot1);
        b2.removeItem(slot2);
        b1.addItem(slot1, itm2);
        b2.addItem(slot2, itm1);
    }
}

