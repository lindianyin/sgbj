#pragma once

#include <string>
#include "json_spirit.h"
#include "const_def.h"
#include "singleton.h"
#include "net.h"

struct CharData;

enum find_type
{
    FIND_EMPTY = 0,         //�հ�
    FIND_DAILY_TASK = 1,    //�ճ������һ���Դ

    FIND_TIMES_START = 100,
    FIND_DAILY_SCORE = 101, //ÿ�ձ����һش���
    FIND_COPY = 102,        //�����һش���
    FIND_TREASURE = 103,    //�ر�ͼ�һش���
    FIND_LEVY = 104,        //�����һش���
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
    int type;//�һ�����
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

