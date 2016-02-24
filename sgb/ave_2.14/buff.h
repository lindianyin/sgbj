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
    int m_type;//�����﹥����߹��߷�
    int m_value;
    time_t m_end_time;
    boost::uuids::uuid m_invalid_timer;
    void start(int cid);
};

//����Ч��
struct CharBuffs
{
    CharData& cdata;
    CharBuff buffs[5];    //�������Ч��
    bool reload;

    int load();
    int addBuff(int type, int value, int invalid_time);
    void refresh();
    int getBuffInfo(json_spirit::Array&);

    CharBuffs(CharData& cd);

    void clearBuff();
};

