#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "net.h"

struct CharData;

enum find_type
{
    FIND_EMPTY = 0,         //空白
    FIND_DAILY_TASK = 1,    //日常任务找回资源

    FIND_TIMES_START = 100,
    FIND_DAILY_SCORE = 101, //每日必做找回次数
    FIND_COPY = 102,        //副本找回次数
    FIND_TREASURE = 103,    //藏宝图找回次数
    FIND_LEVY = 104,        //征收找回次数
};

struct baseFindBack
{
    baseFindBack()
    {
        id = 0;
        type = 0;
        silver = 0;
        char_exp = 0;
        extra = 0;
        cost = 0;
        name = "a findback";
        memo = "a findback memo";
    }
    int id;
    int type;//找回类型
    int silver;
    int char_exp;
    int extra;
    int cost;
    std::string name;
    std::string memo;
    void toObj(json_spirit::Object& obj);
};

class findBackMgr
{
public:
    findBackMgr();
    void getButton(CharData* pc, json_spirit::Array& list);
    void getList(CharData* pc, json_spirit::mObject& o, json_spirit::Object& robj);
    boost::shared_ptr<baseFindBack> getBaseFindBack(int id);
    int findBack(CharData* pc, int id, int cost, json_spirit::Object& robj);
private:
    std::map<int, boost::shared_ptr<baseFindBack> > m_total_find;
};

int ProcessQueryFindBackList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
int ProcessFindBack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

