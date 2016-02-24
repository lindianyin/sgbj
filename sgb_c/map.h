
#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "item.h"
#include "new_combat.hpp"

struct baseStronghold;
struct baseStage;
struct baseMap;
struct CharData;

enum STRONGHOLD_TYPE
{
    STRONGHOLD_TYPE_INIT = 0,
    STRONGHOLD_TYPE_NORMAL = 1,
    STRONGHOLD_TYPE_EXPLORE = 2,
    STRONGHOLD_TYPE_BOX = 3,
    STRONGHOLD_TYPE_CAPTURE = 4,
};

struct Pos
{
    Pos()
    {
        m_x = 0;
        m_y = 0;
    };
    Pos(int x, int y)
    {
        m_x = x;
        m_y = y;
    };
    int m_x;
    int m_y;
};


//关卡武将数据
struct baseStrongholdHeroData
{
    std::string m_name; //姓名
    int m_spic;         //图片
    int m_star;         //星级
    int m_quality;      //品质
    int m_race;         //种族
    int m_level;        //等级
    int m_hp;           //血量
    int m_attack;       //攻击
    int m_defense;      //防御
    int m_magic;        //魔力
    //boost::shared_ptr<specialSkill> m_speSkill;
    void toObj(json_spirit::Object& obj);
};

//地图-场景-关卡
//关卡数据
struct baseStronghold
{
    int m_id;       //关卡唯一id
    int m_level;    //关卡等级
    int m_mapid;    //所属地图
    int m_stageid;  //所属场景
    int m_pos;      //所在位置
    int m_spic;     //关卡图片
    int m_type;     //关卡类型(普通，探索，宝箱，俘虏)
    int m_need_times;    //占领需要的攻打次数
    int m_silver;   //关卡拥有的初始筹码
    int m_cost_gold;//点券通关消耗
    int m_base_exp;
    int m_base_prestige;
    std::string m_name;
    std::string m_chat;//喊话
    std::string m_memo;
    std::string m_tips;

    int m_guide_id;    //打完关卡的对应引导

    //地图上坐标
    Pos m_station;

    //关卡路径
    std::vector<Pos> m_path;

    //关卡战斗指定牌型
    std::vector<int> m_card_rank;

    //战斗属性
    combatAttribute m_combat_attribute;

    //关卡掉落
    boost::shared_ptr<json_spirit::Object> m_loot;
    boost::shared_ptr<Item> m_Item;

    boost::shared_ptr<baseStrongholdHeroData> m_hero;//关卡英雄
    boost::shared_ptr<baseStage> m_baseStage;//关卡所属场景
    ~baseStronghold() {std::cout << "destroying a baseStronghold" << std::endl; }
};

//场景章节
struct baseStage
{
    int m_id;       //场景id(地图内)
    int m_mapid;    //所属地图
    int m_spic;
    int m_minlevel;
    int m_maxlevel;
    std::string m_name;//场景名
    int m_size;
    double m_prestige_fac[4];//声望系数

    std::vector<boost::shared_ptr<baseStronghold> > m_baseStrongholds;//场景关卡
    boost::shared_ptr<baseMap> m_baseMap;//场景所属地图
};

//地图
struct baseMap
{
    int m_id;       //地图唯一id
    std::string m_name;//地图名
    std::string m_memo;
    double m_exp_fac;//经验系数
    std::vector<boost::shared_ptr<baseStage> > m_baseStages;//地图场景章节
};

//玩家关卡进度数据
struct CharStrongholdData
{
    int m_cid;  //角色id
    int m_state;//关卡状态-2代表不存在，-1代表未激活，其他代表总攻击次数
    boost::shared_ptr<baseStronghold> m_baseStronghold;

    CharStrongholdData(int cid)
    :m_cid(cid)
    {};
    bool isPassed();
    int state();
    int exp_reward();
    int prestige_reward(int race);
};

//玩家场景进度数据
struct CharStageData
{
    int m_cid;
    boost::shared_ptr<baseStage> m_baseStage;
    std::vector<boost::shared_ptr<CharStrongholdData> > m_strongholds;//场景关卡
    void save();
};

typedef std::map<int, boost::shared_ptr<CharStageData> > CharMapData;

//玩家进度数据
struct CharTempoData
{
    CharData& m_charData;
    std::map<int, boost::shared_ptr<CharMapData> > CharMapsData;
    CharTempoData(CharData& c)
    :m_charData(c)
    {
    };
    int load(int cid, int loadMap);
    //插入角色关卡进度
    int InitCharTempo(int mapid);
    int update(int stronghold, bool bBroad);
    int Save();
    bool isMapPassed(int mapid);
    boost::shared_ptr<CharStrongholdData> getDestStronghold();//查找目前可打的关卡
    int get_stage_finish_loot(int mapid, int stageid , json_spirit::Object& robj);
    bool check_stage_finish(int mapid, int stageid);
};

class mapMgr
{
public:
    mapMgr();
    int GetStrongholdid(int mapid, int stageid, int pos);//获取关卡id号
    int GetStrongholdPos(int& mapid, int& stageid, int strongholdid);//获取关卡位置
    int GetMapMemo(int mapid, std::string& name, std::string& memo);//获取地图描述
    boost::shared_ptr<baseStronghold> GetBaseStrongholdData(int strongholdid);    //获取基础关卡
    boost::shared_ptr<baseStronghold> GetBaseStrongholdData(int mapid, int stageid, int pos);    //获取基础关卡
    boost::shared_ptr<baseMap> GetBaseMap(int mapid);//获得地图
    boost::shared_ptr<CharStrongholdData> GetCharStrongholdData(int cid, int mapid, int stageid, int strongholdpos);    //获取角色关卡状态
    int combatResult(chessCombat* pCombat);    //战斗结束
private:
    std::map<int, boost::shared_ptr<baseStronghold> > m_base_stronghold_map;//基础关卡数据
    std::vector<boost::shared_ptr<baseMap> > m_base_maps;    //基础地图
};

//显示玩家地图场景进度
int ProcessCharMapTempo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示玩家场景关卡进度
int ProcessCharStageTempo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//显示玩家单个关卡信息
int ProcessCharStronghold(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//开启地图关卡宝箱
int ProcessGetStrongholdBox(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取通关奖励
int ProcessGetStageFinishLoot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//查询通关奖励
int ProcessCheckStageFinish(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//点券通关
int ProcessGoldAttackStronghold(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

