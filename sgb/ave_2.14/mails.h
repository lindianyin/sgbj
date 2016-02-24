#pragma once

#include "net.h"
#include "jobqueue.hpp"
#include "utils_all.h"
#include "worker.hpp"

using namespace net;

enum mail_cmd_enum
{
    mail_cmd_keep_db = 0,
    mail_cmd_get_list = 0x01,
    mail_cmd_query_mail = 0x02,
    mail_cmd_delete_mail = 0x03,
    mail_cmd_delete_mails = 0x04,
    mail_cmd_archive_mail = 0x5,
    mail_cmd_send_mail = 0x6,
    mail_cmd_get_unread = 0x7,
    mail_cmd_get_unread_list = 0x8,
    mail_cmd_set_unread = 0x9,
    mail_cmd_get_gm_question = 0x0a,
    mail_cmd_send_gm_question = 0x0b,
    mail_cmd_get_mail_attach = 0x0c
};

struct mailCmd
{
    int cid;
    int cmd;
    json_spirit::mObject mobj;
    mailCmd();
};

class mailProcesser : public worker<mailCmd>
{
public:
    mailProcesser(jobqueue<mailCmd>& _jobqueue, std::size_t _maxthreads = 1) :
      worker<mailCmd>("mailProcesser",_jobqueue, _maxthreads)
    {
        
    }
    virtual bool work(mailCmd &cmd);       // 在些完成实际任务.
private:
    int m_current_mail_id;
};

