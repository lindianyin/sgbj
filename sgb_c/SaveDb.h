
#include "net.h"
#include "jobqueue.hpp"
#include "utils_all.h"
#include "worker.hpp"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

extern Database& GetDb();

using namespace net;

struct saveDbJob
{
    //std::string sql;
    std::list<std::string> sqls;
    int param[4];
    std::string strParam[2];

    saveDbJob()
    {
        //sql = "";
        memset(param, 0, sizeof(int)*4);
        strParam[0] = "";
        strParam[1] = "";
    }
    saveDbJob(const std::string& s, int param1, int param2, const std::string& sp1, const std::string& sp2)
    {
        if ("" != s)
        {
            sqls.push_back(s);
        }
        param[0] = param1;
        param[1] = param2;
        strParam[0] = sp1;
        strParam[1] = sp2;
    }
};

class SavedbProcess : public worker<saveDbJob>
{
public:
    SavedbProcess(jobqueue<saveDbJob>& _jobqueue, std::size_t _maxthreads = 4) :
      worker<saveDbJob>("saveDb", _jobqueue, _maxthreads)
    {

    }

    virtual bool work(saveDbJob& job);       // 在些完成实际任务.
};

