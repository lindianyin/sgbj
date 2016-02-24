#pragma once

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include "json_spirit.h"
#include "spls_const.h"

using namespace json_spirit;

class CharData;

struct baseState
{
    int id;                //状态唯一id
    int type;            //状态类别: 1主将专属 2/兵种专属 3通用
    int effect_type;    //状态效果 参见  special_state
    int effect_gailv;    //状态概率
    int effect_value;    //状态数值
    std::string name;    //状态名
    std::string memo;    //状态描述
};

//新状态
struct newCharStates
{
    CharData& cdata;
    bool _enable;        //是否开启了
    int _star_level;    //星图等级
    int _stars[8];        //破军，武曲，廉贞，文曲，禄存，巨门，贪狼，紫薇
    int _effects[8];    //状态效果

    int _cur_state[3];    //当前状态 1-8

    int load();            //从数据库加载
    int init();            //首次开启初始化
    void save();        //保存到数据库
    int levelup();        //升级或者斗转星移
    
    int refresh(int type, int pos = 1);        //玩家刷新状态
    int refresh();        //系统自动刷新状态    

    //北斗7星信息
    int getStarInfo(json_spirit::Object& robj);

    //状态刷新信息
    int getStateInfo(json_spirit::Array&);

    //状态刷新信息
    int getCostInfo(json_spirit::Object&);

    newCharStates(CharData& cd);
};

struct npcStrongholdStates
{
    int _cid;
    int _stronghold;
    int _level;            //状态等级
    int _state_num;    //状态个数
    int _cur_state[3];    //当前状态 1-8
    int _effects[3];
    bool m_need_refresh;
    int load();

    int refresh();
    //状态信息
    int getStateInfo(json_spirit::Array&);

    npcStrongholdStates(int cid, int stronghold, int level, int num);
};

class baseStateMgr
{
public:
    baseStateMgr();
    ~baseStateMgr();
    int load();
    //获得基础状态
    boost::shared_ptr<baseState> GetBaseState(int id);
    static baseStateMgr* getInstance();
    //玩家主动刷新状态，指定刷那个
    int refresh(int vip_level, int hero_level, boost::shared_ptr<baseState>* states, int idx /* 1,2,3 */, int type = 1);
    //战斗结束后自动刷新玩家状态
    int refresh(int vip_level, int hero_lv, boost::shared_ptr<baseState>*, int type = 1);
    //刷新关卡状态
    int refreshNpcState(int level, int state_nums, boost::shared_ptr<baseState>* states);

    json_spirit::Object getStateObj(int idx);
    int getAllStates(int hero_level, boost::shared_ptr<baseState>* curStates, json_spirit::Array& allStates);

private:
    static baseStateMgr* m_handle;
    std::map<int, boost::shared_ptr<baseState> > m_base_states;        //基础状态数据

    std::vector<int> m_state_sets[3];        //三个状态集合，用来随机状态

    json_spirit::Object m_state_obj[max_refresh_state_types];        //开放给用户的状态obj，查询所有状态时使用
};

