#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "data.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

enum city_building_type_enum
{
    BUILDING_TYPE_BASE = 0,
    BUILDING_TYPE_CASTLE = 1,
    BUILDING_TYPE_METALLURGY = 2,
    BUILDING_TYPE_SMITHY = 3,
    BUILDING_TYPE_BARRACKS = 4,
    BUILDING_TYPE_COPY = 5,
    BUILDING_TYPE_RACE = 6,
    BUILDING_TYPE_SHENJIANG = 7,
    BUILDING_TYPE_SHENLING = 8,
    BUILDING_TYPE_AUCTION = 9,
    BUILDING_TYPE_MAX = 9,
};

struct base_city_building_info
{
    int type;
    int open_level;
    std::string name;
    int x;
    int y;
};

struct base_castle
{
    int m_level;//城堡等级
    int m_need_level;//升级需求等级
    int m_need_cost;//升级消耗筹码
    int m_min_resident;//居民最小值
    int m_max_resident;//居民最大值
    int m_output_resident;//每次招募居民数
    int m_output_silver;//每次税收产出基数
    int m_rob_defense;//掠夺防御值减少掠夺损失
    int m_attack_per;//城防加成
    int m_defense_per;//城防加成
    int m_magic_per;//城防加成
    int m_hp_per;//城防加成
};

struct char_castle
{
    int m_level;//等级
    int m_resident;//居民数量
    int m_recruit_cd;//招募冷却时间
    int m_levy;//税收领取情况0未领取1已领取
    CharData& m_chardata;//所属玩家
    boost::uuids::uuid _uuid;    //定时器唯一id
    int levelup(json_spirit::Object& robj);//升级城堡（玩家操作）
    int recruit(json_spirit::Object& robj);//招募居民（玩家操作）
    void resident_away();//居民流失（系统定时）
    void cal_add(int& att_per, int& def_per, int& levy_per);// 根据居民百分比获得攻击防御税收加成值
    int gerRobDefense();
    void getDefenseAdd(int& att_per, int& def_per, int& magic_per, int& hp_per);
    int levy_get();
    int levy_left_times();
    int levy(bool cost, json_spirit::Object& robj);//税收以及强制税收（玩家操作）
    void toObj(json_spirit::Object& robj);
    void save();

    char_castle(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

struct base_metallurgy
{
    int m_level;//炼金房等级
    int m_need_level;//升级需求等级
    int m_need_cost;//升级消耗筹码
    double m_compound_add;//合成成功率 加成
    int m_compound_star;//可合成英雄星级
    int m_decompose_star;//可分解英雄星级
    int m_golden_star;//可点金英雄星级
    int m_smelt_cnt;//开放熔炼数量
};

struct char_metallurgy
{
    int m_level;//等级
    CharData& m_chardata;//所属玩家
    boost::uuids::uuid _uuid;    //定时器唯一id
    int levelup(json_spirit::Object& robj);//升级
    double getCompoundAdd();
    int getCompoundMaxStar();
    int getDecomposeMaxStar();
    int getGoldenMaxStar();
    int getSmeltCnt();
    void toObj(json_spirit::Object& robj);
    void save();

    char_metallurgy(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

struct base_smithy
{
    int m_level;//铁匠铺等级
    int m_need_level;//升级需求等级
    int m_need_cost;//升级消耗筹码
    double m_compound_add;//合成成功率 加成
};

struct char_smithy
{
    int m_level;//等级
    CharData& m_chardata;//所属玩家
    int levelup(json_spirit::Object& robj);//升级
    double getCompoundAdd();
    void toObj(json_spirit::Object& robj);
    void save();

    char_smithy(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

struct base_barracks
{
    int m_level;//兵营等级
    int m_need_level;//升级需求等级
    int m_need_cost;//升级消耗筹码
    double m_add;//加成
};

struct char_barracks
{
    int m_level;//等级
    CharData& m_chardata;//所属玩家
    int levelup(json_spirit::Object& robj);//升级
    double getAdd();
    void toObj(json_spirit::Object& robj);
    void save();

    char_barracks(CharData& chardata)
    :m_chardata(chardata)
    {
    }
};

class cityMgr
{
public:
    cityMgr();
    void getButton(CharData* pc, json_spirit::Array& list);
    boost::shared_ptr<base_city_building_info> getBuildingInfo(int type);
    base_castle* getCastle(int level);
    char_castle* getCharCastle(int cid);
    void residentAway();//居民流失（系统定时）
    void resetLevy();//重置税收（系统定时）
    boost::shared_ptr<base_metallurgy> getMetallurgy(int level);
    int getSmeltOpenLevel(int cnt);
    boost::shared_ptr<char_metallurgy> getCharMetallurgy(int cid);
    boost::shared_ptr<base_smithy> getSmithy(int level);
    boost::shared_ptr<char_smithy> getCharSmithy(int cid);
    boost::shared_ptr<base_barracks> getBarracks(int level);
    boost::shared_ptr<char_barracks> getCharBarracks(int cid);
private:
    int m_max_castle_level;
    int m_max_metallurgy_level;
    int m_max_smithy_level;
    int m_max_barracks_level;
    base_castle _base_castle[100];//城堡列表
    std::vector<boost::shared_ptr<base_metallurgy> > base_metallurgy_list;//炼金房列表
    std::vector<boost::shared_ptr<base_smithy> > base_smithy_list;//铁匠铺列表
    std::vector<boost::shared_ptr<base_barracks> > base_barracks_list;//兵营列表
    std::map<int, boost::shared_ptr<char_castle> > m_char_castles;
    std::map<int, boost::shared_ptr<char_metallurgy> > m_char_metallurgys;
    std::map<int, boost::shared_ptr<char_smithy> > m_char_smithys;
    std::map<int, boost::shared_ptr<char_barracks> > m_char_barracks;
    
    std::vector<boost::shared_ptr<base_city_building_info> > base_buildinginfo_list;//城池配置信息
};

//获取城内建筑列表
int ProcessQueryCityBuildingList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//获取城内建筑信息
int ProcessQueryCityBuilding(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//升级城内建筑
int ProcessLevelUpCityBuilding(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//招募居民
int ProcessRecruit(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//招募按钮更新
int ProcessCityRecruitUpdate(json_spirit::mObject& o);
//城堡收税
int ProcessLevy(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
//熔炼刷新按钮更新
int ProcessSmeltRefreshUpdate(json_spirit::mObject& o);

