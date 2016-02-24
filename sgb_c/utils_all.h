#ifndef _UTILS_ALL_H_
#define _UTILS_ALL_H_

#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include <list>
#include <time.h>
#include "json_spirit.h"
#include "item.h"

using namespace std;

#define ERR() std::cout<<"!!!!!!!!!!!!!!!!!error!!!!!!!!!!!!!!!!:"<<__FILE__<<" at line "<<__LINE__<<std::endl<<std::flush

#define DB_ERROR(q)\
    ERR();\
    std::cout<<"sql:"<<q.GetLastQuery()<<std::endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<std::endl

#define CHECK_DB_ERR(q) if (q.GetErrno())\
    {\
        DB_ERROR(q);\
    }

#define DEF_READ_INT_FROM_MOBJ(m, o, f)\
    int m = 0;\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_int();\
        }\
    }

#define DEF_READ_UINT64_FROM_MOBJ(m, o, f)\
    uint64_t m = 0;\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_uint64();\
        }\
    }

#define DEF_READ_STR_FROM_MOBJ(m, o, f)\
    std::string m = "";\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::str_type)\
        {\
            m = o[f].get_str();\
        }\
    }

#define READ_INT_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_int();\
        }\
    }
#define READ_UINT64_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::int_type)\
        {\
            m = o[f].get_uint64();\
        }\
    }

#define READ_REAL_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::real_type)\
        {\
            m = o[f].get_real();\
        }\
    }

#define READ_STR_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::str_type)\
        {\
            m = o[f].get_str();\
        }\
    }

#define READ_OBJ_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::obj_type)\
        {\
            m = o[f].get_obj();\
        }\
    }
#define READ_ARRAY_FROM_MOBJ(m, o, f)\
    if(o.find(f) != o.end())\
    {\
        if (o[f].type() == json_spirit::array_type)\
        {\
            m = o[f].get_array();\
        }\
    }

#define LEX_CAST_STR(x)\
    boost::lexical_cast<std::string>(x)

#define LEX_CAST_INT(x)\
    boost::lexical_cast<int>(x)

struct rwlock {
    int write;
    int read;
};

static inline void
rwlock_init(struct rwlock *lock) {
    lock->write = 0;
    lock->read = 0;
}

static inline void
rwlock_rlock(struct rwlock *lock) {
    for (;;) {
        while(lock->write) {
            __sync_synchronize();
        }
        __sync_add_and_fetch(&lock->read,1);
        if (lock->write) {
            __sync_sub_and_fetch(&lock->read,1);
        } else {
            break;
        }
    }
}

static inline void
rwlock_wlock(struct rwlock *lock) {
    while (__sync_lock_test_and_set(&lock->write,1)) {}
    while(lock->read) {
        __sync_synchronize();
    }
}

static inline void
rwlock_wunlock(struct rwlock *lock) {
    __sync_lock_release(&lock->write);
}

static inline void
rwlock_runlock(struct rwlock *lock) {
    __sync_sub_and_fetch(&lock->read,1);
}

struct compare_pair
{
    typedef std::pair<int, int> value_type;
    bool operator () (const value_type & ls, const value_type &rs)
    {
        return ls.first < rs.first || (ls.first == rs.first && ls.second < rs.second);
    }
};

uint64_t splsTimeStamp();

void init_random_seed();

int my_random(int min, int max);

int myrandom (int i);

double my_random(double min, double max);

void splite_num_random(int total, int min_val, int max_val, int count, std::vector<int>& result_list);

void SetDbname(const std::string& dbname);
std::string GetDbName();

void SetPort(int port);

int GetPort();

int mysqlOption();

int mysqlPort();

void mysqlPort(int port);

void do_sleep(int ms);

bool str_replace(std::string& msg, const std::string& toreplace, const std::string& replacemsg, bool rfind = true);

std::string& str_replace_all(std::string& str, const std::string& old_value, const std::string& new_value);
std::string& str_replace_all_distinct(std::string& str, const std::string& old_value, const std::string& new_value);

std::string getErrMsg(int error_no);

time_t spls_mktime(int year, int month, int day, int hour, int minute);

std::string getAuthKey(int union_id);
std::string getRechargeKey(int union_id);

void setAuthKey(int union_id, const std::string& key);

std::string hexstr(unsigned char *buf, int len);

int getPercent(int now_, int total_);

std::string percent2String(int level, int level2);

std::string int2percent(int value, int div);

std::string time2string(int hour, int min);

int stageIndex(int mapid, int stageid);

void getCurTime(struct tm* t);

//开放规则
struct openRule
{
    int _open_mon;
    int _open_day;
    int _open_week;
    int _open_hour;
    int _open_min;

    openRule(const std::string& mon, const std::string& day, const std::string& week, const std::string& hour, const std::string& min);
};

struct openRules
{
    std::list<openRule> _rules;
    void addRule(const std::string& mon, const std::string& day, const std::string& week, const std::string& hour, const std::string& min);
    openRule* getRule(struct tm&);
};

//发送系统邮件
int sendSystemMail(const std::string& name, int to_id, const std::string& title, const std::string& content, std::string attach = "", int battleId = 0, int event_type = 0, int event_extra = 0);

//把角色名字变成可以点击的链接
std::string MakeCharNameLink(const std::string& name, const std::string& nick = "[]");
//把角色名字变成可以点击的链接(其他地方)
std::string MakeCharNameLink_other(const std::string& name);
//道具名字变成可以点击的链接
std::string MakeGemLink(const std::string& name, int id, int nums);
//英雄名字变成可以点击的链接
std::string MakeHeroLink(const std::string& name, int cid, int id, int star);
//装备名字变成可以点击的链接
std::string MakeEquipLink(const std::string& name, int cid, int id);
//根据品质加颜色
void addColor(std::string& s, int quality);

size_t read_int_array(json_spirit::mObject& obj, const std::string& k, int* pArray, size_t size);
void write_int_array(json_spirit::Object& obj, const std::string& k, int* pArray, size_t size);

void read_int_vector(const std::string& data, std::vector<int>& v);

void lower_str(std::string& str);

inline int getLevelRank(int level)
{
    int rank = (level - 1) / 10 + 1;
    if (rank > 13)
    {
        rank = 13;
    }
    return rank;
}

void my_sys_log(const std::string& msg);

void itemlistToArray(std::list<Item>& loots, json_spirit::Array& getlist);
std::string itemlistToAttach(std::list<Item>& loots);
std::string itemlistToString(std::list<Item>& loots);
std::string itemlistToStringWithLink(std::list<Item>& loots);
int itemlistNeedBagSlot(std::list<Item>& loots, int count = 1);
int itemlistNeedHeroSlot(std::list<Item>& loots);
time_t getZeroTime();

#endif

