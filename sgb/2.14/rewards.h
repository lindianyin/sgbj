
#pragma once

#include <map>
#include <list>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "base_item.h"
#include "net.h"

struct CharData;

enum enum_rewards_type
{
    rewards_type_start = 0,
    rewards_type_boss = 1,
    rewards_type_boss_kill = 2,
    rewards_type_explore = 3,
    rewards_type_race = 4,
    rewards_type_yanhui = 5,
    rewards_type_jtz = 6,
    rewards_type_jt_boss = 7,
    rewards_type_end,
};

struct char_rewards
{
    char_rewards()
    {
        cid = 0;
        type = 0;
        extra = 0;
        state = 0;
    };
    int cid;
    int type;
    int extra;
    int state;
    std::list<Item> m_list;        //奖励列表
    void save();
};

typedef std::map<int, boost::shared_ptr<char_rewards> > CharAllRewards;

class char_rewards_mgr
{
public:
    char_rewards_mgr();
    int get_type(int action_type);
    int get_action_type(int type);
    int get_loot_type(int type);
    boost::shared_ptr<CharAllRewards> getCharAllRewards(int cid);
    boost::shared_ptr<char_rewards> getCharRewards(int cid, int type);
    void updateCharRewards(int cid, int type, int extra, std::list<Item>& getItems);
    int getReward(CharData& cdata, int type, json_spirit::Object& robj);
    bool canGetReward(int cid, int type);
    void getAction(CharData* pc, json_spirit::Array& blist);
private:
    void load();
    std::map<int, boost::shared_ptr<CharAllRewards> > m_char_all_rewards;
};

//领取奖励 cmd ：getCharRewards
int ProcessGetCharRewards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

