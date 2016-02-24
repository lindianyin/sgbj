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
    mail_cmd_send_mail = 0x04,
    mail_cmd_get_unread = 0x05,
    mail_cmd_get_unread_list = 0x06,
    mail_cmd_get_mail_attach = 0x07,
    mail_cmd_get_all_mail_attach = 0x08
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

/**查询邮件列表**/
int ProcessGetMailList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**查询邮件内容**/
int ProcessQueryMailContent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**删除邮件**/
int ProcessDeleteMail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**发送邮件**/
int ProcessSendMail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**批量删除邮件**/
int ProcessDeleteMails(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**查询未读邮件数量**/
int ProcessGetUnread(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**查询未读邮件**/
int ProcessGetUnreadList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**领取邮件附件**/
int ProcessGetMailAttach(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**领取全部邮件附件**/
int ProcessGetAllMailAttach(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

