#pragma once

#include <stdint.h>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"
#include "net.h"
#include "const_def.h"

#define BAG_DEFAULT_SIZE 24
#define MAX_BAG_SIZE 180
#define MAX_BAOSHI_LEVEL 6
#define MAX_BAOSHI_COUNT 99
#define MAX_BAOSHI_ON 5
const int iBaoShiType = 4;
const int iBaoshiCombineCnt = 4;
const int iEuiptBaoshiSlotMax[iMaxQuality] = {1,2,3,4,5,5};
const int iEuiptBaoshiSlotCost[MAX_BAOSHI_ON] = {20,40,60,80,100};


enum item_base_type_enum
{
    ITEM_TYPE_BASE = 0,
    ITEM_TYPE_CURRENCY = 1,
    ITEM_TYPE_HERO_PACK = 2,
    ITEM_TYPE_GEM = 3,
    ITEM_TYPE_EQUIPMENT = 4,
    ITEM_TYPE_HERO = 5,
    ITEM_TYPE_LIBAO = 6,
    ITEM_TYPE_BAOSHI = 7,
    ITEM_TYPE_MAX = 7,
};

enum currency_id_enum
{
    CURRENCY_ID_GOLD = 1,
    CURRENCY_ID_SILVER = 2,
    CURRENCY_ID_EXP = 3,
    CURRENCY_ID_PRESTIGE_BEGIN = 4,
    CURRENCY_ID_PRESTIGE1 = 5,
    CURRENCY_ID_PRESTIGE2 = 6,
    CURRENCY_ID_PRESTIGE3 = 7,
    CURRENCY_ID_PRESTIGE4 = 8,
    CURRENCY_ID_PRESTIGE_END = 9,
    CURRENCY_ID_CHAR_EXP = 10,
    CURRENCY_ID_ARENA_SCORE = 11,
    CURRENCY_ID_BIND_GOLD = 12,
    CURRENCY_ID_SILVER_LEVEL = 13,
    CURRENCY_ID_CHAR_EXP_LEVEL = 14,
    CURRENCY_ID_BIND_GOLD_LEVEL = 15,
    CURRENCY_ID_PRESTIGE_RANDOM = 16,
};

enum gem_id_enum
{
    GEM_ID_HORN = 1,
    GEM_ID_SHENLING_KEY = 2, //神灵塔门票
    GEM_ID_SHENLING_COIN = 3,  //神灵币
    GEM_ID_HERO_GOLD_4 = 4,
    GEM_ID_HERO_GOLD_5 = 5,
    GEM_ID_COPY_COIN = 12,//勇气勋章
    GEM_ID_VIP_CARD = 13,//无上限
    GEM_ID_VIP_CARD_80 = 14,//最高到80%
    GEM_ID_ELITE_HERO_COST_BEGIN = 1000,
    GEM_ID_ELITE_HERO_COST_COMMON = 1016,//通用的神将材料
    GEM_ID_HERO_MATERIAL_BEGIN = 2000,
    GEM_ID_EQUIPMENT_UP_BEGIN = 3000,
    GEM_ID_UPGRADE_STONE = 3001,//强化石
    GEM_ID_BUFF_BEGIN = 4000,
    GEM_ID_BAOSHI_SLOT = 7001,//钻子
};

//道具用途
enum gem_usage_enum
{
    //1喇叭
    //2神灵塔门票
    //3神灵币
    //4英雄点金
    GEM_USAGE_PRESTIGE = 5,//获得声望
    GEM_USAGE_SILVER = 6,//获得筹码
    GEM_USAGE_ELITE_HERO = 7,//召唤神将
    //8英雄熔炼
    //9强化装备
    //10副本商店
    //11宝石
    //12,13,14升级技能
    GEM_USAGE_VIP_EXP = 15,//增加vip经验
    //16竞技场商店
    GEM_USAGE_HERO_EXP = 17,//增加英雄经验
    GEM_USAGE_BUFF = 18,//增加buff
    //19装备开孔
};

enum equip_slot_enum
{
    EQUIP_SLOT_WEAPON = 1,
    EQUIP_SLOT_SHIELD = 2,
    EQUIP_SLOT_CLOTH = 3,
    EQUIP_SLOT_BOOK = 4,
    EQUIP_SLOT_MAX = 4,
};

struct guid
{
    uint16_t m_type;
    uint16_t m_subType;
    uint32_t m_guid;
};

enum bag_base_type_enum
{
    BAG_TYPE_BASE = 0,
    BAG_TYPE_CHAR = 1,
    BAG_TYPE_HERO = 2,
    BAG_TYPE_EQUIPTMENT = 3,
};

struct Item
{
    int type;   //游戏中物品种类item_base_type_enum
    int id;     //type类型中的id=item_base.stype
    int nums;   //数量
    int spic;   //图片id
    int extra;  //额外数据
    int extra2; //额外数据
    double d_extra[4];
    std::string toString(bool withColor = false, int char_level = 0) const;
    Item()
    {
        type = 0;
        id = 0;
        nums = 0;
        spic = 0;
        extra = 0;
        extra2 = 0;
        for (int i = 0; i < 4; ++i)
        {
            d_extra[i] = 0.0;
        }
    }
    Item(int type_, int id_, int nums_, int extra_)
    :type(type_)
    ,id(id_)
    ,nums(nums_)
    ,extra(extra_)
    {
        spic = id;
        extra2 = 0;
        for (int i = 0; i < 4; ++i)
        {
            d_extra[i] = 0.0;
        }
    }
    void toObj(json_spirit::Object& obj);
    void toPointObj(boost::shared_ptr<json_spirit::Object>& p_obj);
    std::string name() const;
};

struct CharHeroData;
struct CharData;
class item_base;
class Equipment;
class Baoshi;

//容器
class Bag_base
{
public:
    Bag_base(int type, size_t s);
    int getType() const {return m_bag_type;}
    size_t getSize() const {return m_size;}
    void setSize(size_t s) {m_size = s;}
    size_t addSize(size_t a);
    bool hasSlot(size_t s) {return (m_size-m_slot_used) >= s;}
    bool isFull() {return m_slot_used >= m_size;}
    size_t getUsed() {return m_slot_used;}
    void swapItem(uint8_t slot1, uint8_t slot2);
    uint8_t addItem(uint8_t slot, boost::shared_ptr<item_base>& , bool notify = true);
    uint8_t addItem(boost::shared_ptr<item_base>& , bool notify = true);
    boost::shared_ptr<item_base> removeItem(uint8_t slot, bool notify = true);
    boost::shared_ptr<item_base> getItem(uint8_t slot);
    int32_t getCount(uint16_t type, uint16_t subType);
    virtual void loadBag() = 0;
    virtual int showBag(int page, int pagenums, json_spirit::Object& robj);
    virtual int showBagEquipments(json_spirit::Array& lists, json_spirit::mObject& o, int& cur_nums);
    virtual int showBagGem(json_spirit::Array& lists, json_spirit::mObject& o, int& cur_nums);
    virtual int showBagBaoshis(json_spirit::Array& lists, json_spirit::mObject& o, int& cur_nums);
    Equipment* getEquipById(int id);
    int bestEquipmentQuality(int etype);
    int bestEquipmentLevel(int etype);
    Baoshi* getBaoshiById(int id);
protected:
    int m_bag_type;//背包类型
    bool m_sorted;//是否排序
    size_t m_size;
    size_t m_slot_used;
    boost::shared_ptr<item_base> m_bagslot[MAX_BAG_SIZE];
};

//背包
class CharBag : public Bag_base
{
public:
    CharBag(CharData& c, size_t s);
    //道具
    int32_t addGem(uint16_t stype, int32_t count, int& err_code, bool sell_if_full = true);
    int32_t getGemCurrency(uint16_t stype);
    int32_t getGemCount(uint16_t stype);
    //整理
    void sortBag();
    void sortInsert(boost::shared_ptr<item_base>& itm);
    int buyBagSlot(int num, json_spirit::Object& robj);
    void loadBag();
    //背包里的内容
    boost::shared_ptr<item_base> getEquipItemById(int id);
    //使用背包内道具
    int openSlotItm(int slot, int nums, json_spirit::Object& robj);
    int getChengzhangLibaoSlot(int libao_id);
    void getBagEquipments(int etype, int quality, int eid, std::list<int>& id_list);

    Baoshi* getBaoshiCanMerge(int base_id, int level, int count);
    boost::shared_ptr<item_base> cloneBaoshi(int base_id, int level, int count);
    int addBaoshi(int base_id, int level, int count);
    int addBaoshiCount(int base_id, int level, int count);

    CharData& m_chardata;
    friend class CharData;
};

//英雄物品容器
class HeroBag : public Bag_base
{
public:
    HeroBag(CharHeroData& c, size_t s);
    CharHeroData* getHero() const;
    void loadBag();

    CharHeroData& m_herodata;
    friend class CharHeroData;
};

//装备物品容器
class EquipmentBag : public Bag_base
{
public:
    EquipmentBag(Equipment& c, size_t s);
    Equipment* getEquipment() const;
    void loadBag();

    Equipment& m_equiptdata;
    friend class Equipment;
};

class item_base
{
public:
    item_base(uint16_t type, uint16_t subType, uint32_t id, int count);
    uint32_t getId() const {return m_guid.m_guid;}
    uint16_t getType() const {return m_guid.m_type;}
    uint16_t getSubType() const {return m_guid.m_subType;}
    uint16_t setSubType(uint16_t subType) { m_guid.m_subType = subType; }
    virtual uint16_t getSpic() const {return 0;}
    virtual std::string getName() const {return "unknow name";}
    virtual std::string getMemo() const {return "unknow memo";}
    virtual int getQuality() const {return 0;}
    virtual int getLevel() const {return 1;}
    Bag_base* getContainer() const {return m_container;}
    int32_t getCount() const {return m_count;}
    int32_t addCount(int32_t a);
    void setSlot(uint8_t slot) {m_changed = true;m_slot = slot;}
    void Clear() {m_count = 0;m_changed = true;}
    uint8_t getSlot() const {return m_slot;}
    void setContainer(Bag_base* container) { m_container = container;m_changed=true;}
    void clearContainer() {m_container = NULL;m_slot = 0;m_changed = true;}
    CharData* getChar() const;
    bool isInBag() const { return m_container != NULL; }
    virtual int32_t maxCount() const {return 1;}
    virtual int32_t sellPrice() const {return 0;}
    virtual void Save() = 0;
    void setChanged() {m_changed = true;}
    void unsetChanged() {m_changed = false;}
protected:
    guid m_guid;//属性
    uint8_t m_slot;//在容器中的位置
    int32_t m_count;//数量
    Bag_base* m_container;//在哪个容器中
    bool m_changed;
};

struct baseGem
{
    bool currency;//是否属于资源
    bool auction;//是否可拍卖
    int id;//基础id
    int usage;//用途
    int spic;
    int extra[3];//使用道具需要的额外参数
    int quality;
    int sellPrice;
    int gold_to_buy;//金币购买价格
    int gold_vip;//金币购买需要vip
    int max_size;
    int invalidTime;//道具失效时间
    std::string name;
    std::string memo;

    //json_spirit::Array canMake;
    //bool canMakeInited;
    //boost::shared_ptr<lootPlaceInfo> m_place;
};

class Gem : public item_base
{
public:
    Gem(uint16_t subtype, uint32_t id, int32_t count);
    bool canAuction() const;
    virtual uint16_t getSpic() const;
    int getUsage() const;
    virtual std::string getName() const;
    virtual std::string getMemo() const;
    virtual int getQuality() const;
    virtual int32_t maxCount() const;
    virtual int32_t sellPrice() const;
    void setInvalidTime(time_t vt = 0);
    int getBaseInvalidTime() const;
    time_t getInvalidTime() const {return m_invalidTime;}
    int getExtra1() const;
    int getExtra2() const;
    int getExtra3() const;
    virtual void Save();
    void toObj(json_spirit::Object& gq);
private:
    boost::shared_ptr<baseGem> m_base;
    time_t m_invalidTime;    //失效时间
};

struct baseEquipment
{
    int id;        //装备基础id
    int type;        //装备位置等同类型 1 武器 2 盾牌3衣服 4书
    int spic;
    int quality;    //品质
    int value;    //基础属性值
    int sellPrice;    //基础出售价格
    std::string name;    //装备名
    std::string memo;    //装备描述
};

class Equipment : public item_base
{
public:
    Equipment(uint16_t subtype, int id, int cid, int level=1, int quality = 0);
    int getEquipType() const;
    virtual uint16_t getSpic() const;
    virtual std::string getName() const;
    virtual std::string getMemo() const;
    virtual int getQuality() const {return m_quality;}
    virtual int getLevel() const {return m_level;}
    int getValue() const {return m_value;}
    int getCompoundSilver() const;
    virtual int32_t sellPrice() const {return m_sellPrice;}
    void updateAttribute(bool real_update = true);
    virtual void Save();
    void toObj(json_spirit::Object& eq, int quality = 0, int level = 0);
    void updateQuality();
    void updateLevel(int level);
    void addBlessValue(int value);
    void clearBlessValue();
    int getBlessValue() {return m_bless_value;}
    int maxBaoshiSlot() {return iEuiptBaoshiSlotMax[m_quality-1];}
    int addBaoshiSlot();
    EquipmentBag m_bag;
    friend class EquipmentBag;
private:
    boost::shared_ptr<baseEquipment> m_base;
    int m_level;
    int m_cid;
    int m_sellPrice;
    int m_quality;    //品质
    int m_value;        //属性值
    int m_bless_value;//祝福值
};

struct baseBaoshi
{
    int id;
    int type;    //宝石类别
    std::string name;    //宝石名字
    std::string memo;    //宝石描述
    int values[MAX_BAOSHI_LEVEL];//宝石对应的效果
    int qualitys[MAX_BAOSHI_LEVEL];//宝石对应的颜色
};

class Baoshi : public item_base
{
public:
    Baoshi(int id, baseBaoshi& base, int level, int count = 1);
    bool canLevelup() const {return m_level < MAX_BAOSHI_LEVEL;}
    int getBaoshiType() const;
    virtual std::string getName() const;
    virtual std::string getMemo() const;
    virtual int getQuality() const {return m_quality;}
    virtual int getLevel() const {return m_level;}
    virtual int32_t maxCount() const{return MAX_BAOSHI_COUNT;}
    virtual int32_t sellPrice() const {return m_level*1000;}
    int getValue() const {return m_value;}
    virtual void Save();
    void toObj(json_spirit::Object& obj, int level = 0);
    void updateLevel(int level);
private:
    const baseBaoshi& m_base;
    int m_level;    //宝石等级
    int m_value;    //属性效果
    int m_quality;    //品质
};

//显示背包
int ProcessShowBag(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//整理背包
int ProcessSortBag(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//卖掉物品
int ProcessSellItem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买背包位
int ProcessBuyBagSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//交换背包位
int ProcessSwapBagSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//使用背包物品
int ProcessOpenBagSlot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//打开背包内礼包
int ProcessOpenBagLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//单独请求装备信息
int ProcessGetEquipInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//单独请求物品信息
int ProcessGetGemInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//单独请求礼包信息
int ProcessGetLibaoInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//请求物品列表
int ProcessShowGems(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//请求基本装备信息
int ProcessGetSysEquipInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取基本道具列表
int ProcessGetSysGemList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买道具
int ProcessBuyGem(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询道具数量
int ProcessGetGemCount(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询道具价格
int ProcessGetGemPrice(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

