#pragma once

#include "bag.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include <string>
#include "attribute.h"

#define MAX_BAOSHI_LEVEL 13

#define MAX_BAOSHI_COUNT 99

const int iFreeYushi = 3;

struct baseNewBaoshi
{
    int type;    //宝石类别
    int atype;    //属性类别

    int cost;    //购买需要的玉石
    int goldCost;//购买需要的金币
    int silverCost;//购买需要的银币

    std::string name;    //宝石名字
    std::string memo;    //宝石描述
    int values[MAX_BAOSHI_LEVEL];        //12级宝石对应的效果

    int qualitys[MAX_BAOSHI_LEVEL];    //12级宝石对应的颜色
    //int exps[MAX_BAOSHI_LEVEL];            //12级宝石需要的经验

    baseAttribute& baseAttr;

    baseNewBaoshi(baseAttribute& a)
    :baseAttr(a)
    {
    }

    void toObj(int level, json_spirit::Object& obj);
    std::string Name_to_Link(int level);
};

class newBaoshi : public iItem
{
public:
    newBaoshi(int id, baseNewBaoshi& base, int level, int exp);

    int level() const;
    int levelup();
    int value () const;
    bool canLevelup() const;
    virtual std::string memo() const;
    virtual std::string name() const;
    virtual void Save();
    void setLevel(int);
    const json_spirit::Object& getObj() const;
    void updateObj();
    int getQuality() const {return m_quality;}
    int32_t maxCount() const {return MAX_BAOSHI_COUNT;}
    virtual int32_t sellPrice() const;
    
    const baseNewBaoshi& m_base;        //基础宝石    
private:
    int m_level;    //宝石等级
    int m_value;    //属性效果
    //int m_exp;        //宝石当前经验
    int m_quality;    //品质

    json_spirit::Object m_obj;

    std::string m_link_name;    //带链接的名字
};

struct baoshi_goods
{
    int type;    //宝石类别
    int level;    //宝石等级
    int goldCostMax;//原价
    int goldCost;//购买需要的金币
    int counts;//宝石数量
};

class newBaoshiMgr
{
public:
    newBaoshiMgr();
    ~newBaoshiMgr();
    baseNewBaoshi* getBaoshi(int type);
    //合并宝石
    int combine(int id1, int id2);
    //购买宝石
    int buyBaoshi(CharData* pc, int& type, json_spirit::Object& robj, int btype=1, int auto_buy = 0);
    //克隆宝石
    boost::shared_ptr<iItem> cloneBaoshi(int type, int level = 1, int count = 1);
    //销毁宝石
    void destroyBaoshi(int id);
    //查询宝石
    boost::shared_ptr<iItem> queryBaoshi(int id);
    //增加宝石
    int addBaoshi(CharData* pc, int type, int level = 1, int count = 1, int reason = 1);
    //注册宝石
    void registerBaoshi(int id, boost::shared_ptr<iItem> bs);

    int newBaoshiId() {return ++m_baoshi_id;}

    //宝石商店
    void load_shop();
    void refresh_shop();
    void save_shop(int change_pos);
    int shop_baoshi(CharData* pc, int pos, json_spirit::Object& robj);
    int baoshishopInfo(json_spirit::Object& robj);
private:
    std::vector<baseNewBaoshi*> m_baseBaoshis;
    std::map<int, boost::shared_ptr<iItem> > m_baoshiMap;

    volatile int m_baoshi_id;
    
    baoshi_goods m_goods[6];//限购宝石列表
};

