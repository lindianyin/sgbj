
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

int main(int argc,char *argv[])
{
    if (argc < 3)
    {
        INFO("add_event db_name event [param1] [param2]");
        return 0;
    }

    Database db("localhost", "c_user", "23rf234", argv[1]);//, CLIENT_INTERACTIVE);
    Query q(db);

    int param1 = 0, param2 = 0, param3 = 0, param4 = 0;

    std::string extra = "";

    if (argc > 3)
    {
        param1 = atoi(argv[3]);
    }
    if (argc > 4)
    {
        param2 = atoi(argv[4]);
    }
    if (argc > 5)
    {
        param3 = atoi(argv[5]);
    }
    if (argc > 6)
    {
        param4 = atoi(argv[6]);
    }
    if (argc > 7)
    {
        extra = argv[7];
    }
    std::string event = argv[2];
    q.execute("insert into schedule_event (id,event,param1,param2,param3,param4,extra,inputTime) values (NULL,'"
                    + event + "',"
                    + boost::lexical_cast<string>(param1) + ","
                    + boost::lexical_cast<string>(param2) + ","
                    + boost::lexical_cast<string>(param3) + ","
                    + boost::lexical_cast<string>(param4) + ",'"
                    + db.safestr(extra) + "',unix_timestamp())");
    return 0;
}

