#pragma once

#include <stdint.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "net.h"

#define BAG_DEFAULT_SIZE 54
#define MAX_BAG_SIZE 180

class Bag;
class newBaoshi;

void swapBag(Bag& b1, uint8_t slot1,Bag& b2, uint8_t slot2);

enum iItem_usage_enum
{
    ITEM_USAGE_NORMAL = 0,
    ITEM_USAGE_HORSE_TRAIN = 5,      //战马训练
    ITEM_USAGE_LOTTERY = 6,           //抽奖
    ITEM_USAGE_EQUIPMENT_SCROLL = 8, //装备卷轴
    ITEM_USAGE_EQUIPMENT_METRIAL = 9,//装备材料
    ITEM_USAGE_CHANGE_CARD = 10,     //变身卡
    ITEM_USAGE_FOR_TRADE = 11,       //贸易商品
    ITEM_USAGE_FOR_TASK = 12,        //任务用品
    ITEM_USAGE_SILVER_CARD = 13,     //兑换银币
    ITEM_USAGE_GOLD_CARD = 14,       //兑换金币
    ITEM_USAGE_YUSHI_CARD = 15,      //兑换玉石
    ITEM_USAGE_PRESTIGE_CARD = 16,   //兑换声望
    ITEM_USAGE_SUPPLY_CARD = 17,     //兑换军粮
    ITEM_USAGE_GONGXUN_CARD = 18,    //兑换功勋
    ITEM_USAGE_TRAIN = 19,            //演兵碎片
    ITEM_USAGE_VIP_EXP_80 = 20,      //VIP经验卡
    ITEM_USAGE_BUFF_BINGLI = 21,     //限时增益加成
    ITEM_USAGE_BUFF_WUGONG = 22,     //限时增益加成
    ITEM_USAGE_BUFF_WUFANG = 23,     //限时增益加成
    ITEM_USAGE_BUFF_CEGONG = 24,     //限时增益加成
    ITEM_USAGE_BUFF_CEFANG = 25,     //限时增益加成
    ITEM_USAGE_VIP_EXP = 26,         //VIP经验卡
    ITEM_USAGE_BOX = 27,              //宝箱
    ITEM_USAGE_LEVY_LING = 28,       //征收令
    ITEM_USAGE_EXPLORE_LING = 29,    //探索令
    ITEM_USAGE_SOUL_LING = 30,       //演兵令
    ITEM_USAGE_TRADE_LING = 31,      //辩才符
    ITEM_USAGE_TRAIN_LING = 32,      //兵法符
    ITEM_USAGE_CORPS_LOTTERY = 33,  //转运符，军团罗生盘
    ITEM_USAGE_FRIEND_LOTTERY = 34, //抽奖券，好友邀请
    ITEM_USAGE_INHERIT = 35,         //传承丹，传承用
    ITEM_USAGE_BAG_KEY = 36,         //仓库钥匙
};

enum iItem_type_enum
{
    iItem_type_gem = 2,
    iItem_type_equipment = 3,
    iItem_type_baoshi = 14,
    iItem_type_quest = 200,
    iItem_type_libao = 201,
    iItem_type_yao = 202,
    iItem_type_cailiao = 203,
    iItem_type_bag = 500
};

struct guid
{
    uint16_t m_type;
    uint16_t m_subType;
    uint32_t m_guid;
};

struct CharGeneralData;
struct CharData;

class iItem
{
public:
    iItem(uint16_t type, uint16_t subType, uint32_t id, int count);
    uint32_t getId() const {return m_guid.m_guid;}
    uint16_t getType() const {return m_guid.m_type;}
    uint16_t getSubtype() const {return m_guid.m_subType;}
    uint16_t setSubType(uint16_t subType) { m_guid.m_subType = subType; }
    virtual uint16_t getSpic() const {return m_guid.m_subType;}
    Bag* getContainer() const {return m_container;}
    int32_t getCount() const {return m_count;}
    int32_t addCount(int32_t a);
    void setSlot(uint8_t slot) {m_changed = true;m_slot = slot;}
    void Clear() {m_count = 0;m_changed = true;}
    uint8_t getSlot() const {return m_slot;}
    void setContainer(Bag* container) { m_container = container;m_changed=true;}
    void clearContainer() {m_container = NULL;m_slot = 0;m_changed = true;}
    boost::shared_ptr<CharGeneralData> getGeneral() const;
    CharData* getChar() const;
    bool isInBag() const { return m_container != NULL; }
    uint8_t getBagSlot() const;
    virtual int32_t maxCount() const {return 1;}
    virtual int32_t sellPrice() const {return 0;}
    virtual std::string name() const {return "unknow name";}
    virtual std::string memo() const {return "unknow memo";}
    virtual time_t getDeleteTime() const {return m_deleteTime;}
    virtual time_t getInvalidTime() const {return m_invalidTime;}
    virtual void Save() = 0;
    virtual int getQuality() const {return 0;}
    void setDeleteTime(time_t dt = 0);
    void setInvalidTime(time_t vt = 0);
    void buyBack() {m_state = 0;m_changed=true;}
    void setChanged() {m_changed = true;}
    void unsetChanged() {m_changed = false;}
protected:
     guid m_guid;
    uint32_t m_owner;

    uint8_t m_slot;            //在容器中的位置
    int32_t m_count;
    Bag* m_container;        //在哪个容器中
    time_t m_deleteTime;    //删除时间
    time_t m_invalidTime;    //失效时间
    uint8_t m_state;
    bool m_changed;
};

struct baseTreasure;

class Gem : public iItem
{
public:
    Gem(uint16_t type, uint16_t subtype, uint32_t id, int32_t count);
    virtual int32_t maxCount() const;
    virtual int32_t sellPrice() const;
    virtual std::string name() const;
    virtual std::string memo() const;
    virtual int getQuality() const;
    virtual void Save();
    int getUsage() const;
    int getValue() const;
    int getBaseInvalidTime() const;
    uint16_t getSpic() const;
#ifdef QQ_PLAT
    int getGoldCost() const;
#endif

private:
    boost::shared_ptr<baseTreasure> m_base;
};

struct CharData;
struct EquipmentData;
struct CharGeneralData;

struct GemCurrency
{
    uint16_t type;
    uint32_t id;
    int32_t count;
    GemCurrency()
    {
        id = 0;
        type = 0;
        count = 0;
    }
};

//背包
class Bag : public iItem
{
public:
    Bag(CharData& c);
    Bag(CharData& c, size_t s);
    void swapItem(uint8_t slot1, uint8_t slot2);
    uint8_t addItem(uint8_t slot, boost::shared_ptr<iItem>& , bool notify = true);
    uint8_t addItem(boost::shared_ptr<iItem>& , bool notify = true);
    boost::shared_ptr<iItem> removeItem(uint8_t slot, bool notify = true);
    boost::shared_ptr<iItem> getItem(uint8_t slot);

    virtual void Save() {}

    int32_t getCount(uint16_t type, uint16_t subType);
    size_t addSize(size_t a);
    int32_t addGem(uint16_t stype, int32_t count, int& err_code, bool sell_if_full = true);
    int32_t getGemCurrency(uint16_t stype);
    int32_t getGemCount(uint16_t stype);
    bool CheckHasEquipt(int base_id);
    int maxEquipLevel(int type, int& id);

    int getBestEquipment(int type, int maxNeedLevel, int quality, int level);

    //返回实际增加
    int addBaoshi(int type, int level, int count);
    newBaoshi* getBaoshiCanMerge(int type, int level, int count);
    int addLibao(int id);
    int showBag(int page, int pagenums, json_spirit::Object& robj);

    bool isFull();
    EquipmentData* getEquipById(int id);
    EquipmentData* getDefaultEquip(int id, int max_level);
    int getMinEnhanceCost(int& eid);

    newBaoshi* getBaoshi(int id);

    boost::shared_ptr<iItem> getEquipItemById(int id);

    int getBestBagEquipments(int level, int type, json_spirit::Object& robj);

    int showBagEquipments(json_spirit::Object& robj);
    int showBagBaoshis(json_spirit::Object& robj);
    int showGeneralBaoshis(json_spirit::Object& robj);
    int baoshiAttrList(json_spirit::Array& alist);

    bool canXiangqian(uint8_t slot, int type);

    //背包整理
    void sortBag();
    //排序插入
    void sortInsert(boost::shared_ptr<iItem>& itm);

    void loadCharBag(int cid);
    int getChengzhangLibaoSlot(int libao_id);
    size_t size() {return m_size;}
    void size(size_t s) {m_size = s;}

    size_t getUsed() {return m_slot_used;}
#ifdef QQ_PLAT
    void login_to_tencent();
    void logout_to_tencent();
#endif

    CharData& m_cdata;
    boost::shared_ptr<CharGeneralData> gd;

    friend class General;
    friend class CharGeneralData;
    friend class ZhenData;
    friend class CharData;
    friend int ProcessCombineBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
    friend int ProcessCombineAllBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
    friend int ProcessChangeBaoshi(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

protected:
    bool m_sorted;        //已经排序
    size_t m_size;
    size_t m_slot_used;
    boost::shared_ptr<iItem> m_bagslot[MAX_BAG_SIZE];
    //货币类道具
    std::map<int, GemCurrency> m_currencys;
};

class selledBag : public Bag
{
public:
    selledBag(CharData& c);
    void load();
    void add(boost::shared_ptr<iItem>& itm);
    //boost::shared_ptr<iItem> remove(int pos);
    void getList(int page, int nums_per_page, json_spirit::Object& robj);
    EquipmentData* getEquipById(int id);
    boost::shared_ptr<iItem> removeByEquipId(int id);
    int buyBack(int type, int id);

private:
    //CharData& m_cdata;
    std::vector<boost::shared_ptr<iItem> > m_selled_list;
};

