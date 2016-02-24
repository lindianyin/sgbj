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
    virtual bool work(mailCmd &cmd);       // ��Щ���ʵ������.
private:
    int m_current_mail_id;
};

/**��ѯ�ʼ��б�**/
int ProcessGetMailList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**��ѯ�ʼ�����**/
int ProcessQueryMailContent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**ɾ���ʼ�**/
int ProcessDeleteMail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**�����ʼ�**/
int ProcessSendMail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**����ɾ���ʼ�**/
int ProcessDeleteMails(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**��ѯδ���ʼ�����**/
int ProcessGetUnread(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**��ѯδ���ʼ�**/
int ProcessGetUnreadList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**��ȡ�ʼ�����**/
int ProcessGetMailAttach(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);
/**��ȡȫ���ʼ�����**/
int ProcessGetAllMailAttach(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

