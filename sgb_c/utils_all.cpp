
#include "stdafx.h"

#include "utils_all.h"
#include <time.h>
#include <boost/random.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <string>
#include <iostream>
#include <pthread.h>

#include "utils_lang.h"

#include "boost/date_time/gregorian/gregorian.hpp"
#include "const_def.h"
#include "data.h"
#include "singleton.h"
#include <cstdlib>      // std::rand, std::srand
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

static std::string g_db_name = "spls";
static std::string g_exe_name = "shhx";
static int m_socket_port = 80;
static int g_mysql_option = 0;
static int g_mysql_port = 3306;

static const std::string g_char_link = "$S<font color='#FFFF00'><A HREF=\"event:{'cmd':'showRole','name':'$N'}\"><U>$n</U></A></font>";
static const std::string g_char_link_other = "<font color='#FFFF00'><A HREF=\"event:$N\"><U>$n</U></A></font>";
static const std::string strGemLink = "<A HREF=\"event:{'type':$T,'count':$C,'cmd':'getGemInfo'}\"><U>$N</U></A>";
static const std::string strHeroLink = "<A HREF=\"event:{'cid':$C,'id':$T,'star':$S,'cmd':'getHeroInfo'}\"><U>$N</U></A>";
static const std::string strEquipLink = "<A HREF=\"event:{'cid':$C,'id':$T,'cmd':'getEquipInfo'}\"><U>$N</U></A>";

using namespace std;

Database& GetDb();
void InsertSaveDb(const std::string& sql);

pthread_key_t thread_random_gen_key;
void close_thread_random_gen(void* pgen)
{
    boost::mt19937* pg = reinterpret_cast<boost::mt19937*>(pgen);
    if (pg)
    {
        delete pg;
    }
}

void SetDbname(const std::string& dbname)
{
    g_db_name = dbname;
}

std::string GetDbName()
{
    return g_db_name;
}

void SetExeName(const std::string& ename)
{
    g_exe_name = ename;
}

std::string GetExeName()
{
    return g_exe_name;
}

int mysqlOption()
{
    return g_mysql_option;
}

int mysqlPort()
{
    return g_mysql_port;
}

void mysqlPort(int port)
{
    g_mysql_port = port;
}

void SetPort(int port)
{
    m_socket_port = port;
}

int GetPort()
{
    return m_socket_port;
}

void init_random_seed()
{
    srand((int)time(0));
    std::srand ( unsigned ( std::time(0) ) );
    pthread_key_create (&thread_random_gen_key, close_thread_random_gen);
}

boost::mt19937* get_random_generator()
{
    boost::mt19937* pgen = reinterpret_cast<boost::mt19937*>(pthread_getspecific (thread_random_gen_key));
    if (!pgen)
    {
        pgen = new boost::mt19937(42u);//, CLIENT_INTERACTIVE);
        pthread_setspecific (thread_random_gen_key, pgen);
        pgen->seed(time(NULL));    //初始化随机数
    }
    return pgen;
}

int my_random(int mins, int maxs)
{
    if (mins >= maxs)
    {
        return mins;
    }
    boost::mt19937* pgen = get_random_generator();
    if (pgen)
    {
        boost::uniform_int<> dist(mins, maxs);
        boost::variate_generator<boost::mt19937&, boost::uniform_int<> > die(*pgen, dist);
        return die();
    }
    else
    {
        ERR();
        return mins + rand()%(maxs- mins + 1);
    }
}

int myrandom (int i)
{
    return my_random(0, i - 1);
}

double my_random(double mins, double maxs)
{
    int mins_int = (int)(10000.0 * mins);
    int maxs_int = (int)(10000.0 * maxs);
    int ret = my_random(mins_int, maxs_int);
    return (double)(ret/10000.0);
    //return (maxs > mins) ? (mins + (double)(g_uni() * (maxs + 1 - mins))) : mins;
}

//把total拆分成[min_val,max_val]区间的count个数
void splite_num_random(int total, int min_val, int max_val, int count, std::vector<int>& result_list)
{
    //cout << "splite_num_random:total=" << total << ",min_val=" << min_val << ",max_val=" << max_val << ",count=" << count << endl;
    if (count < 1 || min_val > max_val || min_val * count > total || max_val * count < total)
        return;
    int left_total = total;
    if (count > 1)
    {
        int left_cnt = count - 1;
        for (int i = 0; i < count-1; ++i)
        {
            int rand_tmp = my_random(min_val,left_total);
            left_total -= rand_tmp;
            //随机出结果不能大于区间上限
            while (rand_tmp > max_val)
            {
                rand_tmp -= max_val;
                left_total += max_val;
            }
            //剩余结果必须足够剩下的数来随机
            while (left_total < (left_cnt*min_val))
            {
                --rand_tmp;
                ++left_total;
            }
            //剩余结果必须不超过剩余数量上限
            while (left_total > (left_cnt*max_val))
            {
                ++rand_tmp;
                --left_total;
            }
            result_list.push_back(rand_tmp);
            --left_cnt;
        }
    }
    result_list.push_back(left_total);
}

void do_sleep(int ms)
{
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC_);
    xt.nsec += ms%1000*1000*1000;
    xt.sec += ms/1000;
    boost::thread::sleep(xt);
}

bool str_replace(std::string& msg, const std::string& toreplace, const std::string& replacemsg, bool rfind)
{
    size_t pos = rfind ? msg.rfind(toreplace):msg.find(toreplace);
    if (pos != std::string::npos)
    {
        msg.replace(pos, toreplace.length(), replacemsg);
        return true;
    }
    else
    {
        std::cout<<"str replace fail:"<<msg<<","<<toreplace<<","<<replacemsg<<std::endl;
        return false;
    }
}

string& str_replace_all(string& str, const string& old_value, const string& new_value)
{
    while(true)   {
        string::size_type   pos(0);
        if(   (pos=str.find(old_value))!=string::npos   )
            str.replace(pos,old_value.length(),new_value);
        else   break;
    }
    return   str;
}

string& str_replace_all_distinct(string& str, const string& old_value, const string& new_value)
{
    for(string::size_type   pos(0);   pos!=string::npos;   pos+=new_value.length())
    {
        if(   (pos=str.find(old_value,pos))!=string::npos   )
            str.replace(pos,old_value.length(),new_value);
        else   break;
    }
    return   str;
}

uint64_t splsTimeStamp()
{
    struct timeval _tstart;
    gettimeofday(&_tstart, NULL);
    return 1000000*_tstart.tv_sec + _tstart.tv_usec;
}

std::string getDate()
{
    using namespace boost::gregorian;
    date d(day_clock::local_day());
    return to_iso_extended_string(d);
}

/*
Member    Meaning    Range
tm_sec    seconds after the minute    0-61*
tm_min    minutes after the hour    0-59
tm_hour    hours since midnight    0-23
tm_mday    day of the month    1-31
tm_mon    months since January    0-11
tm_year    years since 1900
tm_wday    days since Sunday    0-6
tm_yday    days since January 1    0-365
tm_isdst    Daylight Saving Time flag    */

time_t spls_mktime(int year, int month, int day, int hour, int minute)
{
    struct   tm   tm1;
    tm1.tm_year = year - 1900;
    tm1.tm_mon = month - 1;
    tm1.tm_mday = day;
    tm1.tm_hour = hour;
    tm1.tm_min = minute;
    tm1.tm_sec = 0;

    return mktime(&tm1);
}

void setAuthKey(int union_id, const std::string& key)
{
    return;
}

std::string hexstr(unsigned char *buf, int len)
{
    const char *set = "0123456789abcdef";
    char str[65], *tmp;
    unsigned char *end;
    if (len > 32)
        len = 32;

    end = buf + len;
    tmp = &str[0];
    while (buf < end)
    {
        *tmp++ = set[ (*buf) >> 4 ];
        *tmp++ = set[ (*buf) & 0xF ];
        buf ++;
    }
    *tmp = 0;
    return std::string(str);
}

int getPercent(int now_, int total_)
{
    if (!total_)
    {
        return 0;
    }
    return 100 * now_ /total_;
}

void getCurTime(struct tm* t)
{
    time_t t_now = time(NULL);
    localtime_r(&t_now, t);
}

openRule::openRule(const std::string& mon, const std::string& day, const std::string& week, const std::string& hour, const std::string& min)
{
    if("*" == mon)
    {
        _open_mon = -1;
    }
    else
    {
        _open_mon = atoi(mon.c_str());
    }
    if("*" == day)
    {
        _open_day = -1;
    }
    else
    {
        _open_day = atoi(day.c_str());
    }
    if("*" == week)
    {
        _open_week = -1;
    }
    else
    {
        _open_week = atoi(week.c_str());
    }
    if("*" == hour)
    {
        _open_hour = -1;
    }
    else
    {
        _open_hour = atoi(hour.c_str());
    }
    if("*" == min)
    {
        _open_min = -1;
    }
    else
    {
        _open_min = atoi(min.c_str());
    }
}

void openRules::addRule(const std::string& mon, const std::string& day, const std::string& week, const std::string& hour, const std::string& min)
{
    //cout<<"add rule "<<szSeason<<","<<hour<<endl;
    //cout<<"rules.size()="<<_rules.size()<<endl;
    openRule rule(mon, day, week, hour, min);
    bool bFind = false;
    for (std::list<openRule>::iterator it = _rules.begin(); it != _rules.end(); ++it)
    {
        //cout<<"!!!! add rule "<<day<<","<<hour<<endl;
        openRule& rule_ = *it;
        if ((rule_._open_mon == rule._open_mon) &&
            (rule_._open_day == rule._open_day) &&
            (rule_._open_week == rule._open_week))
        {
            bFind = true;
            if (rule_._open_hour > rule._open_hour || (rule_._open_hour == rule._open_hour && rule_._open_min > rule._open_min))
            {
                //cout<<"!!!! insert rule "<<day<<","<<hour<<endl;
                _rules.insert(it, rule);
                return;
            }
        }
        else if (bFind)
        {
            //cout<<"!!!! 222 insert rule "<<day<<","<<hour<<endl;
            _rules.insert(it, rule);
            return;
        }
    }
    //cout<<"!!!! push_back rule "<<day<<","<<hour<<endl;
    _rules.push_back(rule);
}

openRule* openRules::getRule(struct tm& time_now)
{
    openRule* pRet = NULL;
    for (std::list<openRule>::iterator it = _rules.begin(); it != _rules.end(); ++it)
    {
        openRule& rule_ = *it;
        if ((rule_._open_mon == -1 || rule_._open_mon == time_now.tm_mon) &&
            (rule_._open_day == -1 || rule_._open_day == time_now.tm_mday) &&
            (rule_._open_week == -1 || rule_._open_week == time_now.tm_wday))
        {
            pRet = &rule_;
            if (pRet->_open_hour > time_now.tm_hour
                || (pRet->_open_hour == time_now.tm_hour && pRet->_open_min > time_now.tm_min))
            {
                return pRet;
            }
        }
    }
    return pRet;
}

std::string percent2String(int level, int level2)
{
    //cout<<"toString:"<<level<<" and "<<level2<<endl;
    while (level2 >= 100)
    {
        level2 -= 100;
        ++level;
    }
    std::string ret = LEX_CAST_STR(level);
    if (level2 <= 0)
    {
        ;
    }
    else if (level2 < 10)
    {
        ret = ret + ".0" + LEX_CAST_STR(level2);
    }
    else
    {
        ret = ret + "." + LEX_CAST_STR(level2);
    }
    //cout<<"return:"<<ret<<endl;
    return ret;
}

//value = 15, div= 1000 -> 15/1000 = 1.5%
std::string int2percent(int value, int div)
{
    //if (value >= div)
    //{
    //    return "100%";
    //}
    //else
    {
        int z = (value * 100) / div;
        std::string ret = LEX_CAST_STR(z);
        int x = (value * 100 % div);
        if (0 == x)
        {
            return ret + "%";
        }
        else
        {
            x = x / 100;
            return ret + "." + LEX_CAST_STR(x) + "%";
        }
    }
}

//把角色名字变成可以点击的链接(聊天框)
std::string MakeCharNameLink(const std::string& name, const std::string& nick)
{
    std::string link = g_char_link;
    if (nick != "[]")
    {
        str_replace(link, "$S", "#" + nick + "#");
    }
    else
    {
        str_replace(link, "$S", "");
    }
    str_replace(link, "$N", name);
    str_replace(link, "$n", name);
    return link;
}

//把角色名字变成可以点击的链接(其他地方)
std::string MakeCharNameLink_other(const std::string& name)
{
    std::string link = g_char_link_other;
    str_replace(link, "$N", name);
    str_replace(link, "$n", name);
    return link;
}

//道具名字变成可以点击的链接
std::string MakeGemLink(const std::string& name, int type, int nums)
{
    std::string link = strGemLink;
    str_replace(link, "$N", name);
    str_replace(link, "$T", LEX_CAST_STR(type));
    str_replace(link, "$C", LEX_CAST_STR(nums));
    return link;
}

//英雄名字变成可以点击的链接
std::string MakeHeroLink(const std::string& name, int cid, int id, int star)
{
    std::string link = strHeroLink;
    str_replace(link, "$N", name);
    str_replace(link, "$T", LEX_CAST_STR(id));
    str_replace(link, "$S", LEX_CAST_STR(star));
    str_replace(link, "$C", LEX_CAST_STR(cid));
    return link;
}

//装备名字变成可以点击的链接
std::string MakeEquipLink(const std::string& name, int cid, int id)
{
    std::string link = strEquipLink;
    str_replace(link, "$N", name);
    str_replace(link, "$T", LEX_CAST_STR(id));
    str_replace(link, "$C", LEX_CAST_STR(cid));
    return link;
}

//根据品质加颜色
void addColor(std::string& s, int quality)
{
    switch (quality)
    {
        case 1:
            s = "<font color=\"#ffffff\">" + s + "</font>";
            break;
        case 2:
            s = "<font color=\"#00FF0C\">" + s + "</font>";
            break;
        case 3:
            s = "<font color=\"#008AFF\">" + s + "</font>";
            break;
        case 4:
            s = "<font color=\"#f000ff\">" + s + "</font>";
            break;
        case 5:
            s = "<font color=\"#F1C336\">" + s + "</font>";
            break;
        case 6:
        default:
            s = "<font color=\"#ff0000\">" + s + "</font>";
            break;
    }
}

std::string time2string(int hour, int min)
{
    std::string timex;
    if (min >= 60)
    {
        hour++;
        min -= 60;
    }
    if (hour < 10)
    {
        timex = "0" + LEX_CAST_STR(hour);
    }
    else
    {
        timex = LEX_CAST_STR(hour);
    }
    timex += ":";
    if (min < 10)
    {
        timex += "0" + LEX_CAST_STR(min);
    }
    else
    {
        timex += LEX_CAST_STR(min);
    }
    return timex;
}

int stageIndex(int mapid, int stageid)
{
    return(mapid-1)*5 + stageid;
}

size_t read_int_array(json_spirit::mObject& obj, const std::string& k, int* pArray, size_t size)
{
    json_spirit::mArray results_array;
    READ_ARRAY_FROM_MOBJ(results_array, obj, k);

    size_t geted = 0;

    if (results_array.size() < size)
    {
        return 0;
    }

    for (json_spirit::mArray::iterator it = results_array.begin(); it != results_array.end(); ++it)
    {
        if ((*it).type() != json_spirit::int_type)
        {
            break;
        }
        (*pArray) = (*it).get_int();
        ++geted;
        if (geted >= size)
        {
            return geted;
        }
        ++pArray;
    }
    return geted;
}

void write_int_array(json_spirit::Object& obj, const std::string& k, int* pArray, size_t size)
{
    json_spirit::Array list;
    for (size_t i = 0; i < size; ++i)
    {
         list.push_back(*(pArray + i));
    }
    obj.push_back( Pair(k, list) );
}

void read_int_vector(const std::string& data, std::vector<int>& v)
{
    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(",");
    tokenizer tok(data, sep);
    tokenizer::iterator it = tok.begin();
    while (it != tok.end())
    {
        v.push_back(atoi(it->c_str()));
        ++it;
    }
}

void lower_str(std::string& str)
{
    for(unsigned short loop=0;loop < str.size();loop++)
    {
        str[loop]=tolower(str[loop]);
    }
}

void my_sys_log(const std::string& msg)
{
    InsertSaveDb("insert into sys_log set msg='" +GetDb().safestr(msg) + "',input=unix_timestamp()");
}

void itemlistToArray(std::list<Item>& loots, json_spirit::Array& getlist)
{
    std::list<Item>::iterator it = loots.begin();
    while (it != loots.end())
    {
        json_spirit::Object getobj;
        it->toObj(getobj);
        getlist.push_back(getobj);
        ++it;
    }
}

std::string itemlistToAttach(std::list<Item>& loots)
{
    json_spirit::Array list;
    std::list<Item>::iterator it = loots.begin();
    while (it != loots.end())
    {
        json_spirit::Object getobj;
        getobj.push_back( Pair("type", it->type) );
        getobj.push_back( Pair("id", it->id) );
        getobj.push_back( Pair("count", it->nums) );
        getobj.push_back( Pair("extra", it->extra) );
        getobj.push_back( Pair("extra2", it->extra2) );
        getobj.push_back( Pair("fac_a", it->d_extra[0]) );
        getobj.push_back( Pair("fac_b", it->d_extra[1]) );
        getobj.push_back( Pair("fac_c", it->d_extra[2]) );
        getobj.push_back( Pair("fac_d", it->d_extra[3]) );
        list.push_back(getobj);
        ++it;
    }
    return json_spirit::write(list);
}

std::string itemlistToString(std::list<Item>& loots)
{
    std::string getStr = "";
    std::list<Item>::iterator it = loots.begin();
    while (it != loots.end())
    {
        if (getStr == "")
        {
            getStr = it->toString(true);
        }
        else
        {
            getStr += "," + it->toString(true);
        }
        ++it;
    }
    return getStr;
}

std::string itemlistToStringWithLink(std::list<Item>& loots)
{
    std::string getStr = "";
    std::list<Item>::iterator it = loots.begin();
    while (it != loots.end())
    {
        std::string getItem = it->toString(true);
        if(it->type == ITEM_TYPE_EQUIPMENT)
        {
            getItem = MakeEquipLink(getItem, 0, it->id);
        }
        else if(it->type == ITEM_TYPE_HERO)
        {
            getItem = MakeHeroLink(getItem, 0, it->id, it->extra);
        }
        else
        {
            ++it;
            continue;
        }
        if (getStr == "")
        {
            getStr = getItem;
        }
        else
        {
            getStr += "," + getItem;
        }
        ++it;
    }
    return getStr;
}

int itemlistNeedBagSlot(std::list<Item>& loots, int count)
{
    int need_slot_num = 0;
    std::list<Item>::iterator it = loots.begin();
    while (it != loots.end())
    {
        if (it->type == ITEM_TYPE_EQUIPMENT)
        {
            need_slot_num += (it->nums * count);
        }
        else if(it->type == ITEM_TYPE_LIBAO)
        {
            need_slot_num += (it->nums * count);
        }
        else if(it->type == ITEM_TYPE_GEM)
        {
            boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(it->id);
            if (tr.get() && !tr->currency)
            {
                int32_t max_c = tr->max_size;
                if (max_c > 1)
                {
                    int need = (it->nums * count) / max_c;
                    if (it->nums % max_c > 0)
                        ++need;
                    need_slot_num += need;
                }
                else
                {
                    need_slot_num += (it->nums * count);
                }
            }
        }
        ++it;
    }
    return need_slot_num;
}

int itemlistNeedHeroSlot(std::list<Item>& loots)
{
    int need_slot_num = 0;
    std::list<Item>::iterator it = loots.begin();
    while (it != loots.end())
    {
        if (it->type == ITEM_TYPE_HERO)
        {
            ++need_slot_num;
        }
        ++it;
    }
    return need_slot_num;
}

time_t getZeroTime()
{
    time_t timex = time(NULL);
    struct tm tm;
    struct tm *t = &tm;
    localtime_r(&timex, t);
    t->tm_hour = 0;
    t->tm_min = 0;
    t->tm_sec = 0;
    return mktime(t);
}

