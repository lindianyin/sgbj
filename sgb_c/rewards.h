
#pragma once

#include <map>
#include <list>
#include "json_spirit.h"
#include "boost/smart_ptr/shared_ptr.hpp"
#include "item.h"
#include "net.h"

struct CharData;

enum enum_rewards_type
{
    REWARDS_TYPE_START = 0,
    REWARDS_TYPE_ARENA = 1,
    REWARDS_TYPE_PK = 2,
    REWARDS_TYPE_BOSS = 3,
    REWARDS_TYPE_BOSS_KILL = 4,
    REWARDS_TYPE_WEEK_RANKING = 5,
    REWARDS_TYPE_END,
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

class rewardsMgr
{
public:
    rewardsMgr();
    boost::shared_ptr<CharAllRewards> getCharAllRewards(int cid);
    boost::shared_ptr<char_rewards> getCharRewards(int cid, int type);
    void updateCharRewards(int cid, int type, int extra, std::list<Item>& getItems);
    int getReward(CharData& cdata, int type, json_spirit::Object& robj);
    bool canGetReward(int cid, int type);
    void getButton(CharData* pc, json_spirit::Array& list);
private:
    std::map<int, boost::shared_ptr<CharAllRewards> > m_char_all_rewards;
};

//领取奖励 cmd ：getCharRewards
int ProcessGetCharRewards(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

