#pragma once

#include "net.h"
#include "jobqueue.hpp"
#include "utils_all.h"
#include "worker.hpp"

using namespace net;

enum char_db_cmd_enum
{
    db_cmd_load_account = 0x01,
    db_cmd_load_char = 0x02,
    db_cmd_keep_db = 0x3,
    db_cmd_load_char_gag = 0x4,
    db_cmd_query_account_score = 0x5,
    db_cmd_convert_account_score = 0x6,
    db_cmd_char_online = 0x7,
    db_cmd_char_offline = 0x8
};

struct dbCmd
{
    int _cid;
    int _cmd;
    int _param1;
    int _param2;
    std::string _account;
};

class charDbProcesser : public worker<dbCmd>
{
public:
    charDbProcesser(jobqueue<dbCmd>& _jobqueue, std::size_t _maxthreads = 1) :
      worker<dbCmd>("charDbProcesser",_jobqueue, _maxthreads)
    {
        
    }
    virtual bool work(dbCmd &cmd);       // 在些完成实际任务.
};

