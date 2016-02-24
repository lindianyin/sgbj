#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "new_combat.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

//神灵塔数据
struct baseShenling
{
    int m_id;       //id
    int m_level;    //层数
    int m_silver;   //初始筹码
    std::string m_chat;
    //掉落
    std::list<Item> m_Item_list;
    boost::shared_ptr<baseStrongholdHeroData> m_hero;//驻守英雄
    void toObj(json_spirit::Object& obj);
};

//玩家神灵塔数据
struct CharShenling
{
    int m_cid;//角色id
    int m_sid;//当前闯的层数(week_reset)
    int m_magics[3];//对应技能
    void load();
    void refreshSkill();
    void reset();
    void save();
};

//神灵塔商店物品
struct baseShenlingGoods
{
    int id;
    int cost;
    Item m_item;
};

//副本
class shenlingMgr
{
public:
    shenlingMgr();
    //每周重置所有层数
    void weekUpdate();
    boost::shared_ptr<baseShenling> getShenlingById(int sid);
    boost::shared_ptr<CharShenling> getCharShenling(int cid);
    int getCharShenlingInfo(int cid, json_spirit::Object& robj);
    int getShenlingList(int cid, json_spirit::Object& robj);
    int refreshSkill(int cid);
    int reset(int cid, bool needgold = false);
    int buyTimes(int cid);
    int combatResult(chessCombat* pCombat);    //战斗结束
    boost::shared_ptr<baseShenlingGoods> GetBaseShenlingGoods(int id);
    int getShenlingShop(json_spirit::mObject& o, json_spirit::Object& robj);
private:
    std::vector<boost::shared_ptr<baseShenling> > m_shenling;    //全部神灵塔数据
    std::map<int, boost::shared_ptr<CharShenling> > m_char_shenling;    //全部玩家神灵塔数据
    std::map<int, boost::shared_ptr<baseShenlingGoods> > m_base_shenling_goods; //神灵塔商店商品
};

//获取神灵塔列表
int ProcessGetShenlingList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取神灵塔数据
int ProcessGetShenlingInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//刷新神灵塔技能
int ProcessRefreshSkill(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//重置神灵塔
int ProcessResetShenling(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买挑战次数
int ProcessBuyShenlingTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//请求神灵塔商店
int ProcessQueryShenlingShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//购买神灵塔商品
int ProcessBuyShenlingShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

