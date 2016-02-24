#pragma once

#include <string>
#include <map>
#include "boost/smart_ptr/shared_ptr.hpp"

#include "combat_def.h"
#include "json_spirit.h"
#include "spls_const.h"
#include <boost/uuid/uuid.hpp>

using namespace json_spirit;

class CharData;

struct CharBuff
{
    int m_type;//兵力物攻物防策攻策防
    int m_value;
    time_t m_end_time;
    boost::uuids::uuid m_invalid_timer;
    void start(int cid);
};

//增益效果
struct CharBuffs
{
    CharData& cdata;
    CharBuff buffs[5];    //玩家增益效果
    bool reload;

    int load();
    int addBuff(int type, int value, int invalid_time);
    void refresh();
    int getBuffInfo(json_spirit::Array&);

    CharBuffs(CharData& cd);

    void clearBuff();
};

