
#ifndef _EXPLORE_H_
#define _EXPLORE_H_

#include <vector>
#include <list>
#include <map>
#include <boost/cstdint.hpp>

#include <string>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "json_spirit.h"

const int iExploreBuyGoldMax = 20;
const int iExploreCostLevel = 4;
const int iExploreBuyGold[iExploreCostLevel][2] =
{
    {10, 2},        //小于10次2金币
    {25, 5},    //小于25次5金币
    {45, 10},    //小于45次10金币
    {10000, 20}    //大于45次20金币
};
const int iFreeExploreTimes = 3;

class CharData;

//地点
enum explore_places
{
    miaoyu = 1,//庙宇
    junzhai,//军寨
    guangchang,//广场
    kezhan,//客栈
    matou,//码头
    jishi,//集市
    kufang,//库房
    yamen,//衙门
    cunluo,//村落
    kuangjing,//矿井
    tianshuge//天书阁
};

//探索地点
struct explore_place
{
    int id;
    int spic;
    int skill_id;
    int needlevel;
    double silver_fac;
    std::string name;
    std::string memo;
};

//探索组合奖励
struct explore_reward
{
    std::vector<int> combo;
    int type;
    int id;
    int mapid;
    int need;
};

typedef std::list<int> exploreplace_list;

class exploreMgr
{
public:
    static exploreMgr* getInstance();
    int reload();
    int Save(int cid);
    int Explore(int cid, int pid, json_spirit::Object& robj);
    int buyExploreLing(CharData* pc, json_spirit::Object& robj);
    int ExploreRefresh(int cid, bool cost_gold = false);
    bool CheckPlaceGetAlready(int pid, boost::shared_ptr<exploreplace_list> p_list);
    std::string GetRewardName(int type, int id);
    std::string GetPlaceName(int pid);
    int GetGoldRefreshGold(int cid);
    int GetComboPos(int cid, int pid);
    boost::shared_ptr<explore_reward> CheckCombo(int cid, int pid, int num);
    std::list<boost::shared_ptr<explore_reward> > GetExploreReward();
    boost::shared_ptr<explore_place> GetBaseExplorePlace(int pid);
    boost::shared_ptr<exploreplace_list> GetHasExplore(int cid);
    boost::shared_ptr<exploreplace_list> GetCanExplore(int cid);
    //删除角色
    int deleteChar(int cid);
private:
    static exploreMgr* m_handle;
    std::list<boost::shared_ptr<explore_reward> > m_base_explore_reward;//探索组合奖励表
    std::map<int, boost::shared_ptr<explore_place> > m_base_explore_list;//探索地点基础表
    std::map<int, boost::shared_ptr<exploreplace_list> > m_char_has_explore_list;//玩家已探索列表
    std::map<int, boost::shared_ptr<exploreplace_list> > m_char_can_explore_list;//玩家可探索列表
};

#endif

