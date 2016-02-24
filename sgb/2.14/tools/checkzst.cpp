
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include <boost/lexical_cast.hpp>

#include <map>
#include <iostream>

using namespace std;

#if 0

#crontab 加入

#每天早上5点刷新
01 5 * * * root /usr/sbin/spls_event spls daily_reset
#每个小时恢复军令2个
01 * * * * root /usr/sbin/spls_event spls recover_ling 2
#每分钟将部分角色数据存盘
*/1 * * * * root /usr/sbin/spls_event spls save_db

#endif

#define INFO(x) cout<<x<<endl

#define CHECK_DB_ERR(q) if (q.GetErrno())\
    {\
         cout<<"sql:"<<q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<endl;\
    }

#define LEX_CAST_STR(x) boost::lexical_cast<std::string>(x)

struct abase_general_soul
{
    int level;
    int cost;
    int total_cost;
};

int getTotalGet(int cid, Database& db)
{
    int total_get = 0;
    Query q(db);
    q.get_result("SELECT sum(hnums) FROM  `admin_count_smost` WHERE  `cid` =" + LEX_CAST_STR(cid) + " AND  `treasure_id` =4012 and `stype`='1' and `type`!=101");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        total_get = q.getval();
    }
    q.free_result();

    return total_get;
}

int getCurCount(int cid, Database& db)
{
    int count = 0;
    Query q(db);
    q.get_result("select sum(nums) from char_treasures where tid=4012 and cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        count = q.getval();
    }
    q.free_result();

    return count;
}

int getTotalCost(int cid, Database& db, abase_general_soul abg[])
{
    int total = 0;
    Query q(db);
    q.get_result("SELECT soul FROM  `char_generals` where soul>0 and cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        if (level <= 100)
        {
            total += abg[level-1].total_cost;
        }
    }
    q.free_result();
    return total;
}

int main(int argc,char *argv[])
{
    if (argc < 2)
    {
        std::string exe_name = argv[0];
        INFO(exe_name<<" db_name");
        return 0;
    }

    Database db("localhost", "c_user", "23rf234", argv[1]);//, CLIENT_INTERACTIVE);
    Query q(db);

    abase_general_soul m_general_souls[100];
    for (int i = 0; i < 100; ++i)
    {
        m_general_souls[i].level = i + 1;
        m_general_souls[i].cost = 1;
    }

    q.get_result("select level,cost from base_general_soul_cost where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        if (level >= 1 && level <= 100)
        {
            m_general_souls[level-1].cost = q.getval();
        }
    }
    q.free_result();

    for (int lv = 1; lv <= 100; ++lv)
    {
        m_general_souls[lv-1].total_cost = 0;
    }
    for (int lv = 1; lv <= 100; ++lv)
    {
        for (int add_lv = lv; add_lv <= 100; ++add_lv)
        {
            m_general_souls[add_lv-1].total_cost += m_general_souls[lv-1].cost;
        }
    }

    std::map<int,int> cids;
    q.get_result("select cid,nums from char_treasures where tid=4012 order by nums desc limit 5");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        int count = q.getval();

        cids[cid] = count;
    }
    q.free_result();

    q.get_result("SELECT cid FROM  `char_generals` ORDER BY  `char_generals`.`soul` DESC LIMIT 5");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int cid = q.getval();
        cids[cid] = 1;
    }
    q.free_result();

    int err = 0;
    for (std::map<int,int>::iterator it = cids.begin(); it != cids.end(); ++it)
    {
        int cid = it->first;

        int total_cost = getTotalCost(cid, db, m_general_souls);
        int total_get = getTotalGet(cid, db);
        int total_cur = getCurCount(cid, db);

        int diff = total_cost + total_cur - total_get;
        if (diff)
        {
            INFO("cid "<<cid<<","<<diff<<"="<<(total_cost+total_cur)<<"-"<<total_get);
            ++err;
        }
    }

    if (err == 0)
    {
        INFO("it is ok!");
    }
    else
    {
        INFO(" !!!!!!! error !!!!!!");
    }
    return 0;
}

