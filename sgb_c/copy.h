#pragma once

#include "data.h"
#include "net.h"
#include "json_spirit.h"
#include "new_combat.hpp"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

using namespace net;

//精英地图章节数据
struct baseCopyMap
{
    int m_id;//地图id
    std::string m_name;//地图名字
    std::list<Item> m_finish_reward;//通关奖励
    std::list<Item> m_perfect_reward;//完美通关奖励
    void load();
};

//精英数据
struct baseCopy
{
    int m_id;       //副本唯一id
    int m_level;    //副本等级
    int m_mapid;    //所属地图
    int m_spic;     //副本图片
    int m_silver;   //副本拥有的初始筹码
    int m_openLevel;
    std::string m_name;
    std::string m_chat;
    //副本掉落
    std::list<Item> m_Item_list;
    boost::shared_ptr<baseStrongholdHeroData> m_hero;//关卡英雄
};

//玩家副本数据
struct CharCopyData
{
    boost::shared_ptr<baseCopy> m_baseCopy;

    int m_cid;//角色id
    int m_copyid;
    int m_result;//闯关评价(保存历史状态)
    int m_can_attack;//可攻击次数
    int m_can_attack_max;//最大可攻击次数
    void reset();
    void save();
};

//玩家地图副本数据
struct CharMapCopyData
{
    int m_mapid;//地图id
    int m_cid;//角色id
    std::list<boost::shared_ptr<CharCopyData> > m_char_copy_list;//副本队列
    int load();
    void checkFinish();
};

typedef std::map<int, boost::shared_ptr<CharMapCopyData> > m_char_map_copy;//玩家各地图副本数据

//副本商店物品
struct baseCopyGoods
{
    int id;
    int cost;
    int need_copy_map;
    Item m_item;
};

//副本
class copyMgr
{
public:
    copyMgr();
    //每日重置所有攻击次数
    void dailyUpdate();
    boost::shared_ptr<baseCopy> getCopyById(int copyid);
    boost::shared_ptr<CharCopyData> getCharCopy(int cid, int mapid, int copyid);
    boost::shared_ptr<CharMapCopyData> getCharCopys(int cid, int mapid);
    int getCharCopyMapList(int cid, json_spirit::Object& robj);
    int getCharCopyList(int cid, int mapid, json_spirit::Object& robj);
    //章节副本是否全通
    bool isCharMapCopyPassed(int cid, int mapid);
    int getCharCurMap(int cid);
    //初始化玩家副本
    int initCharMapCopy(int cid, int mapid);
    int ResetCopy(int cid, int mapid, int copyid);
    int AddCopyTimes(int cid, int mapid, int copyid);
    int combatResult(chessCombat* pCombat);    //战斗结束
    boost::shared_ptr<baseCopyGoods> GetBaseCopyGoods(int id);
    int getCopyShop(int cid, json_spirit::mObject& o, json_spirit::Object& robj);
    int GetCopyFinishReward(int cid, int mapid, int type, json_spirit::Object& robj);
private:
    int m_max_mapid;
    std::vector<boost::shared_ptr<baseCopyMap> > m_copy_maps;    //全部地图章节
    std::vector<boost::shared_ptr<baseCopy> > m_copys;    //全部地图副本队列
    std::map<int, boost::shared_ptr<m_char_map_copy> > m_char_copys;    //全部玩家地图副本队列
    std::map<int, boost::shared_ptr<baseCopyGoods> > m_base_copy_goods; //副本商店商品
};

//获取副本地图列表
int ProcessGetCopyMapList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取某章节副本列表
int ProcessGetCopyList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取副本具体信息
int ProcessGetCopyInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//重置某副本
int ProcessResetCopy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//增加某副本攻击次数
int ProcessAddCopyTimes(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//副本商店
int ProcessQueryCopyShop(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessBuyCopyShopGoods(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//领取副本地图通关宝箱
int ProcessGetCopyFinishReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

