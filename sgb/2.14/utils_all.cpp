
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
#include "base_item.h"
#include "spls_const.h"
#include "data.h"
#include "singleton.h"
#include "libao.h"
#include <cstdlib>      // std::rand, std::srand

static std::string g_db_name = "spls";
static std::string g_exe_name = "shhx";
static int m_socket_port = 80;
static int g_mysql_option = 0;
static int g_mysql_port = 3306;

static const std::string g_char_link = "<font color='#FFFF00'><A HREF=\"event:{'cmd':'showRole','name':'$N'}\"><U>$n</U></A></font>";
static const std::string g_char_link_other = "<font color='#FFFF00'><A HREF=\"event:$N\"><U>$n</U></A></font>";

static const std::string strTreasureLink = "<A HREF=\"event:{'id':$T,'cmd':'getGemInfo'}\"><U>$N</U></A>";

extern std::string strExpendMsg;

using namespace std;

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

int my_random (int i)
{
    return my_random(0, i - 1);
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

double my_random(double mins, double maxs)
{
    int mins_int = (int)(10000.0 * mins);
    int maxs_int = (int)(10000.0 * maxs);
    int ret = my_random(mins_int, maxs_int);
    return (double)(ret/10000.0);
    //return (maxs > mins) ? (mins + (double)(g_uni() * (maxs + 1 - mins))) : mins;
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

//募兵消耗
int costSilver(int hp, int level)
{
    if (level >= 100)
    {
        return (7 * hp + 5)/10;
    }
    else if (level >= 80)
    {
        return (hp + 1)/2;
    }
    else if (level >= 60)
    {
        return (7*hp + 10)/20;
    }
    else if (level >= 40)
    {
        return (2*hp + 5)/10;
    }
    else if (level >= 20)
    {
        return (hp + 5)/10;
    }
    else
    {
        return (2*hp + 25)/50;
    }
}

std::string Item::name() const
{
    switch (type)
    {
        case item_type_silver:
            return strSilver;
        case item_type_gold:
            return strGold;
        case item_type_ling:
            return strLing;
        case item_type_treasure:
        case item_type_treasure_level:
            {
                boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
                if (tr.get())
                {
                    return tr->name;
                }
            }
            break;
        case item_type_silver_map:
        case item_type_silver_level:
            return strSilver;

        case item_type_general:
            {
                boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral(id);
                if (tr.get())
                {
                    return tr->m_name;
                }
            }
        case item_type_general_baowu:
            return strGeneralTreasure;
        case item_type_prestige:
            return strPrestige;
        case item_type_exp:
            return strExp;
        case item_type_baoshi:
        {
            baseNewBaoshi* tr = Singleton<newBaoshiMgr>::Instance().getBaoshi(id);
            if (tr)
            {
                return tr->name;
            }
        }
        case item_type_equipment:
        {
            boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
            if (tr.get())
            {
                return tr->name;
            }
            break;
        }
        case item_type_zhen:    //阵型
        {
            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(id);
            if (bz.get())
            {
                return bz->m_name;
            }
            break;
        }
        case item_type_skill:    //技能
        {
            boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(id);
            if (bs.get())
            {
                return bs->name;
            }
            break;
        }
        case item_type_libao:    //礼包
        {
            baseLibao* pl = libao_mgr::getInstance()->getBaselibao(id);
            if (pl)
            {
                return pl->m_name;
            }
            break;
        }
        default:
            break;
    }
    return "unknow";
}

std::string Item::toString(bool withColor, int char_level) const
{
    switch (type)
    {
        case item_type_silver:
            {
                if (withColor)
                {
                    return "<font color=\"#FFFF00\">" + strSilver + strCounts + LEX_CAST_STR(nums) + "</font>";
                }
                else
                {
                    return strSilver + strCounts + LEX_CAST_STR(nums);
                }
            }
        case item_type_gold:
            {
                if (withColor)
                {
                    return "<font color=\"#FFFF00\">" + strGold + strCounts + LEX_CAST_STR(nums) + "</font>";
                }
                else
                {
                    return strGold + strCounts + LEX_CAST_STR(nums);
                }
            }
        case item_type_ling:
            {
                if (withColor)
                {
                    return "<font color=\"#FFFF00\">" + strLing + strCounts + LEX_CAST_STR(nums) + "</font>";
                }
                else
                {
                    return strLing + strCounts + LEX_CAST_STR(nums);
                }
            }
        case item_type_treasure:
        case item_type_treasure_level:
            {
                boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
                if (tr.get())
                {
                    if (withColor)
                    {
                        std::string name = tr->name + strCounts + LEX_CAST_STR(nums);
                        addColor(name, tr->quality);
                        return name;
                    }
                    else
                    {
                        return tr->name + strCounts + LEX_CAST_STR(nums);
                    }
                }
            }
            break;
        case item_type_silver_map:
            return strSilver;
        case item_type_silver_level:
            {
                if (char_level > 0)
                {
                    return strSilver + strCounts + LEX_CAST_STR(nums * char_level);
                }
                else
                {
                    return strSilver;
                }
            }

        case item_type_general:
            {
                boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral(id);
                if (tr.get())
                {
                    if (withColor)
                    {
                        return "<font color=\"#ffffff\">" + (nums > 1 ? (tr->m_name + strCounts + LEX_CAST_STR(nums)) :tr->m_name) + "</font>";
                    }
                    else
                    {
                        return nums > 1 ? (tr->m_name + strCounts + LEX_CAST_STR(nums)) :tr->m_name;
                    }
                }
            }
        case item_type_general_baowu:
            if (withColor)
            {
                return "<font color=\"#ff0000\">" +strGeneralTreasure + strCounts + LEX_CAST_STR(nums) + "</font>";
            }
            else
            {
                return strGeneralTreasure + strCounts + LEX_CAST_STR(nums);
            }
        case item_type_prestige:
            {
                if (withColor)
                {
                    return "<font color=\"#FFFF00\">" + strPrestige + strCounts + LEX_CAST_STR(nums) + "</font>";
                }
                else
                {
                    return strPrestige + strCounts + LEX_CAST_STR(nums);
                }
            }
        case item_type_exp:
            return strExp + strCounts + LEX_CAST_STR(nums);
        case item_type_baoshi:
        {
            baseNewBaoshi* tr = Singleton<newBaoshiMgr>::Instance().getBaoshi(id);
            if (tr)
            {
                if (withColor)
                {
#if 0
                    switch (tr->quality)
                    {
                        case 0:
                            return "<font color=\"#ffffff\">" + tr->name + "</font>";
                        case 1:
                            return "<font color=\"#74D3FB\">" + tr->name + "</font>";
                        case 2:
                            return "<font color=\"#00FF0C\">" + tr->name + "</font>";
                        case 3:
                            return "<font color=\"#FFFF00\">" + tr->name + "</font>";
                        case 4:
                            return "<font color=\"#ff0000\">" + tr->name + "</font>";
                        case 5:
                        default:
                            return "<font color=\"#f000ff\">" + tr->name + "</font>";
                    }
#else
                return "<font color=\"#74D3FB\">" + tr->name + "</font>";
#endif
                }
                else
                {
                    return tr->name;
                }
            }
        }
        case item_type_equipment:
        {
            boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
            if (tr.get())
            {
                if (withColor)
                {
                    std::string name = (nums > 1 ? (tr->name + strCounts + LEX_CAST_STR(nums)) :tr->name);
                    addColor(name, tr->quality);
                    return name;
                }
                else
                {
                    return nums > 1 ? (tr->name + strCounts + LEX_CAST_STR(nums)) :tr->name;
                }
            }
            break;
        }
        case item_type_libao:    //礼包
        {
            baseLibao* p = libao_mgr::getInstance()->getBaselibao(id);
            if (p != NULL)
            {
                return nums > 1 ? (p->m_name + strCounts + LEX_CAST_STR(nums)) :p->m_name;
            }
            break;
        }
        default:
            break;
    }
    return "unknow";
}

void Item::toObj(json_spirit::Object& obj)
{
    std::string n = "";
    obj.push_back( Pair("type", type) );
    obj.push_back( Pair("count", nums) );
    obj.push_back( Pair("id", id) );
    obj.push_back( Pair("fac", fac) );
    int quality = 0;
    switch (type)
    {
        case item_type_silver:
        case item_type_silver_map:
        case item_type_silver_level:
            n = strSilver;
            break;
        case item_type_gold:
            n = strGold;
            break;
        case item_type_ling:
            n = strLing;
            break;
        case item_type_treasure:
        case item_type_treasure_level:
            {
                boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(id);
                if (tr.get())
                {
                    n = tr->name;
                    quality = tr->quality;
                    obj.push_back( Pair("spic", tr->spic) );
                }
            }
            break;
        case item_type_general:
            {
                boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral(id);
                if (tr.get())
                {
                    n = tr->m_name;
                    obj.push_back( Pair("spic", tr->m_spic) );
                }
                break;
            }
        case item_type_general_baowu:
            n =strGeneralTreasure;
            break;
        case item_type_prestige:
        case item_type_prestige_level:
            n =strPrestige;
            break;
        case item_type_exp:
            n =strExp;
            break;
        case item_type_baoshi:
        {
            baseNewBaoshi* tr = Singleton<newBaoshiMgr>::Instance().getBaoshi(id);
            if (tr)
            {
                n = tr->name;
                if (fac > MAX_BAOSHI_LEVEL || fac < 1)
                {
                    quality = 1;
                }
                else
                {
                    quality = tr->qualitys[fac-1];
                }
                obj.push_back( Pair("level", fac) );
            }
            break;
        }
        case item_type_equipment:
        {
            boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(id);
            if (tr.get())
            {
                n = tr->name;
                quality = tr->quality;
            }
            break;
        }
        case item_type_zhen:    //阵型
        {
            boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(id);
            if (bz.get())
            {
                n = bz->m_name;
                obj.push_back( Pair("level", nums) );
            }
            break;
        }
        case item_type_skill:    //技能
        {
            boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(id);
            if (bs.get())
            {
                n = bs->name;
                obj.push_back( Pair("level", nums) );
            }
            break;
        }
        case item_type_libao:    //礼包
        {
            baseLibao* p = libao_mgr::getInstance()->getBaselibao(id);
            if (p != NULL)
            {
                n = p->m_name;
                quality = p->m_quality;
                obj.push_back( Pair("spic", p->m_spic) );
            }
            break;
        }
    }
    obj.push_back( Pair("name", n) );
    if (quality > 0)
    {
        obj.push_back( Pair("quality", quality) );
    }
}

int getRefreshStateCost(int mapid, int& silver, int& gold)
{
    if (mapid > max_map_id)
    {
        mapid = max_map_id;
    }
    else if (mapid < 2)
    {
        mapid = 2;
    }
    silver =iRefreshStateSilver[mapid-1];
    gold = iRefreshStateGold;
    return 0;
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

openRule::openRule(const std::string& szSeason, const std::string& mon, const std::string& day, const std::string& week, const std::string& hour, const std::string& min)
{
    if("*" == szSeason)
    {
        _open_season = -1;
    }
    else
    {
        _open_season = atoi(szSeason.c_str());
    }
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

void openRules::addRule(const std::string& szSeason, const std::string& mon, const std::string& day, const std::string& week, const std::string& hour, const std::string& min)
{
    //cout<<"add rule "<<szSeason<<","<<hour<<endl;
    //cout<<"rules.size()="<<_rules.size()<<endl;
    openRule rule(szSeason, mon, day, week, hour, min);
    bool bFind = false;
    for (std::list<openRule>::iterator it = _rules.begin(); it != _rules.end(); ++it)
    {
        //cout<<"!!!! add rule "<<szSeason<<","<<hour<<endl;
        openRule& rule_ = *it;
        if ((rule_._open_season == rule._open_season) &&
            (rule_._open_mon == rule._open_mon) &&
            (rule_._open_day == rule._open_day) &&
            (rule_._open_week == rule._open_week))
        {
            bFind = true;
            if (rule_._open_hour > rule._open_hour || (rule_._open_hour == rule._open_hour && rule_._open_min > rule._open_min))
            {
                //cout<<"!!!! insert rule "<<szSeason<<","<<hour<<endl;
                _rules.insert(it, rule);
                return;
            }
        }
        else if (bFind)
        {
            //cout<<"!!!! 222 insert rule "<<szSeason<<","<<hour<<endl;
            _rules.insert(it, rule);
            return;
        }
    }
    //cout<<"!!!! push_back rule "<<szSeason<<","<<hour<<endl;
    _rules.push_back(rule);
}

openRule* openRules::getRule(struct tm& time_now, int season)
{
    openRule* pRet = NULL;
    for (std::list<openRule>::iterator it = _rules.begin(); it != _rules.end(); ++it)
    {
        openRule& rule_ = *it;
        if ((rule_._open_season == -1 || rule_._open_season == season) &&
            (rule_._open_mon == -1 || rule_._open_mon == time_now.tm_mon) &&
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

std::string percentToString(int level, int level2)
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

static volatile int level_cost_inited = 0;
static uint64_t level_cost[iMaxCharLevel] = {};

static void init_level_cost()
{
    for (int lv = 1; lv <= iMaxCharLevel; ++lv)
    {
        uint64_t r = 0;
        if (lv <= 8)
        {
            r = lv * 31 * 2 / 3 + 55;
        }
        else if (lv <= 15)
        {
            r = 7 * lv * lv + 7* lv-230;
        }
        else if (lv <= 20)
        {
            r = 10* lv * lv + 10* lv-700;
        }
        else if (lv <= 40)
        {
            r = 9 * lv * lv - 35 * lv + 400;
        }
        else if (lv <= 60)
        {
            r = 20 * lv * lv + 20 * lv -11000;
        }
        else if (lv <= 80)
        {
            r = 50 * lv * lv + 50 * lv -110000;
        }
        else
        {
            r = 100 * lv * lv + 100* lv-410000;
        }
        level_cost[lv-1] = r;
    }
    level_cost_inited = 1;
}

uint64_t get_level_cost(int lv)
{
    if (lv >= 1 && lv <= iMaxCharLevel)
    {
        if (!level_cost_inited)
        {
            init_level_cost();
        }
        return level_cost[lv-1];
    }
    return (uint64_t)(-1);
}

//关卡经验，地图id和关卡等级
int getStrongholdExp(int mapid, int slevel)
{
    return 4*slevel + 10 + 80*mapid;
}

//把角色名字变成可以点击的链接(聊天框)
std::string MakeCharNameLink(const std::string& name)
{
    std::string link = g_char_link;
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
std::string MakeTreasureLink(const std::string& name, int id)
{
    std::string link = strTreasureLink;
    str_replace(link, "$N", name);
    str_replace(link, "$T", LEX_CAST_STR(id));
    return link;
}

//根据品质加颜色
void addColor(std::string& s, int quality)
{
    switch (quality)
    {
        case 0:
            s = "<font color=\"#ffffff\">" + s + "</font>";
            break;
        case 1:
            s = "<font color=\"#00FF0C\">" + s + "</font>";
            break;
        case 2:
            s = "<font color=\"#008AFF\">" + s + "</font>";
            break;
        case 3:
            s = "<font color=\"#f000ff\">" + s + "</font>";
            break;
        case 4:
            s = "<font color=\"#F1C336\">" + s + "</font>";
            break;
        case 5:
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

std::string treasure_expend_msg(int type, int count)
{
    boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(type);
    if (tr.get())
    {
        std::string msg = strExpendMsg;
        str_replace(msg, "$T", tr->name, true);
        str_replace(msg, "$C", LEX_CAST_STR(count), true);
        return msg;
    }
    else
    {
        return "";
    }
}

int WashCal(int pre_val, int add_min, int min, int add, int full_val, double per)
{
    //低于保底必加
    if ((pre_val + add) < add_min)
        return (pre_val + add);
    int max = (pre_val + add) > full_val ? full_val : (pre_val + add);
    //提升点数/总区间=提升概率
    //方程关系add / (max-min) + per = add / (max-new_min);
    int interval = max - min;
    int new_min = max - (int)(add * interval / (add + per * interval));
    return my_random(new_min,max);
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
        getobj.push_back( Pair("fac", it->fac) );
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

