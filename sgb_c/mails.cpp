
#include "mails.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_all.h"
#include "json_spirit.h"
#include "errcode_def.h"
#include "data.h"
#include "new_combat.hpp"
#include "text_filter.h"

extern int ProcessKeepDb(json_spirit::mObject& o);
Database& GetDb();
extern void InsertSaveDb(const std::string& sql, int param1, int param2, const std::string& sp1, const std::string& sp2);
extern void InsertSaveDb(const std::string& sql);
extern void InsertMailcmd(mailCmd&);

/**查询邮件列表**/
int ProcessGetMailList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_list;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

/**查询邮件内容**/
int ProcessQueryMailContent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_query_mail;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

/**删除邮件**/
int ProcessDeleteMail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int purpose = 2;
    READ_INT_FROM_MOBJ(purpose,o,"purpose");
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_delete_mail;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**发送邮件**/
int ProcessSendMail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_send_mail;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**查询未读邮件数量**/
int ProcessGetUnread(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_unread;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**查询未读邮件**/
int ProcessGetUnreadList(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }

    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_unread_list;
    InsertMailcmd(cmd);
    return HC_SUCCESS_NO_RET;
}

/**领取邮件附件**/
int ProcessGetMailAttach(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_mail_attach;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

/**领取全部邮件附件**/
int ProcessGetAllMailAttach(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    mailCmd cmd;
    cmd.mobj = o;
    cmd.mobj["name"] = pc->m_name;
    cmd.cid = pc->m_id;
    cmd.cmd = mail_cmd_get_all_mail_attach;

    InsertMailcmd(cmd);

    return HC_SUCCESS_NO_RET;
}

//读取邮件列表 0x1
int getMaillist(int cid,  const std::string& cname, int purpose, int page, int pageOfNums)
{
    json_spirit::Object robj;
    if (page <= 0)
    {
        page = 1;
    }
    if (pageOfNums == 0)
    {
        pageOfNums = 10;
    }
    int cur_nums = 0;
    int first_nums = pageOfNums * (page - 1)+ 1;
    int last_nums = pageOfNums * page;

    json_spirit::Array mlist;
    Query q(GetDb());
    std::string sql = "select cm.id,cm.input,cm.title,c.name,cm.unread,cm.type,cm.attach_state,(unix_timestamp(cm.input) + 1296000 - unix_timestamp())  from char_mails as cm left join charactors as c";
    if (purpose == 1)//收件箱
    {
        sql += (" on cm.`from`=c.id where cm.cid=" + LEX_CAST_STR(cid) + " and (archive='2' or archive='3')");
    }
    else if(purpose == 2)//发件箱
    {
        sql += (" on cm.`cid`=c.id where cm.from=" + LEX_CAST_STR(cid) + " and (archive='1' or archive='3')");
    }
    sql += " order by unread desc,input desc";
    q.get_result(sql);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        ++cur_nums;
        if (cur_nums >= first_nums && cur_nums <= last_nums)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", q.getval()) );
            obj.push_back( Pair("time", q.getstr()) );
            obj.push_back( Pair("title", q.getstr()) );
            obj.push_back( Pair("name", q.getstr()) );
            obj.push_back( Pair("unread", q.getval()) );
            obj.push_back( Pair("type", q.getval()) );
            obj.push_back( Pair("attach_state", q.getval()) );
            obj.push_back( Pair("left_time", q.getval()) );
            mlist.push_back(obj);
        }
    }
    q.free_result();

    robj.push_back( Pair("cmd", "getMailList") );
    robj.push_back( Pair("s", 200) );
    robj.push_back( Pair("purpose", purpose) );

    robj.push_back( Pair("list", mlist) );

    int maxpage = cur_nums/pageOfNums + ((cur_nums%pageOfNums) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", pageOfNums) );
    robj.push_back( Pair("page", pageobj) );

    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

//读取邮件详细信息 0x2
int queryMailContent(int cid, const std::string& cname, int purpose, int id)
{
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "queryMail") );
    robj.push_back( Pair("purpose", purpose) );
    Query q(GetDb());
    std::string sql = "select cm.id,cm.input,cm.title,c.name,cm.content,cm.type,cm.attach,cm.attach_state from char_mails as cm left join charactors as c";
    if (purpose == 1)//收件箱
    {
        sql += (" on cm.`from`=c.id where cm.cid=" + LEX_CAST_STR(cid) + " and (archive='2' or archive='3') and cm.id=" + LEX_CAST_STR(id));
    }
    else if(purpose == 2)//发件箱
    {
        sql += (" on cm.`cid`=c.id where cm.from=" + LEX_CAST_STR(cid) + " and (archive='1' or archive='3') and cm.id=" + LEX_CAST_STR(id));
    }
    q.get_result(sql);
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        json_spirit::Object info;
        robj.push_back( Pair("s", 200) );
        info.push_back( Pair("id", q.getval()) );
        info.push_back( Pair("time", q.getstr()) );
        info.push_back( Pair("title", q.getstr()) );
        info.push_back( Pair("name", q.getstr()) );
        info.push_back( Pair("content", q.getstr()) );
        info.push_back( Pair("type", q.getval()) );
        robj.push_back( Pair("info", info) );
        //附件
        std::string attach = q.getstr();
        int attach_state = q.getval();
        q.free_result();
        if ((attach_state == 1 || attach_state == 2) && attach != "")
        {
            json_spirit::mValue value;
            json_spirit::read(attach, value);

            if (value.type() == json_spirit::array_type)
            {
                json_spirit::Array getlist;
                json_spirit::mArray list = value.get_array();
                json_spirit::mArray::iterator it = list.begin();
                while (it != list.end())
                {
                    if ((*it).type() != json_spirit::obj_type)
                    {
                        ++it;
                        continue;
                    }
                    json_spirit::mObject& tmp_obj = (*it).get_obj();
                    int type = 0, id = 0, count = 0, extra = 0, extra2 = 0;
        			double fac_a = 0.0, fac_b = 0.0, fac_c = 0.0, fac_d = 0.0;
                    READ_INT_FROM_MOBJ(type,tmp_obj,"type");
                    READ_INT_FROM_MOBJ(id,tmp_obj,"id");
                    READ_INT_FROM_MOBJ(count,tmp_obj,"count");
                    READ_INT_FROM_MOBJ(extra,tmp_obj,"extra");
                    Item item(type, id, count, extra);
                    READ_INT_FROM_MOBJ(extra2,tmp_obj,"extra2");
					READ_REAL_FROM_MOBJ(fac_a,tmp_obj,"fac_a");
					READ_REAL_FROM_MOBJ(fac_b,tmp_obj,"fac_b");
					READ_REAL_FROM_MOBJ(fac_c,tmp_obj,"fac_c");
					READ_REAL_FROM_MOBJ(fac_d,tmp_obj,"fac_d");
                    item.extra2 = extra2;
                    item.d_extra[0] = fac_a;
                    item.d_extra[1] = fac_b;
                    item.d_extra[2] = fac_c;
                    item.d_extra[3] = fac_d;
                    //cout << "type = " << type << " id = " << id << " count = " << count << endl;
                    json_spirit::Object getobj;
                    item.toObj(getobj);
                    getlist.push_back(getobj);
                    ++it;
                }
                robj.push_back( Pair("attach", getlist) );
                robj.push_back( Pair("attach_state", attach_state) );
            }
        }
        if (purpose == 1)
        {
            if (!q.execute("update char_mails set unread=0 where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id)))
            {
                CHECK_DB_ERR(q);
            }
        }
    }
    else
    {
        robj.push_back( Pair("s", HC_ERROR) );
        q.free_result();
    }
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

//删除邮件 0x3
int deleteMail(int cid, const std::string& cname, int purpose, int id)
{
    int ret = 0, value = 2;
    std::string s_value = "";
    Query q(GetDb());
    if (purpose == 1)//收件箱
    {
        value = 2;
        s_value = "cid";
    }
    else if(purpose == 2)//发件箱
    {
        value = 1;
        s_value = "from";
    }
    //archive信件保存状态（1为发件人保存2为收件人保存3为双方保存）
    if (!q.execute("update char_mails set archive=archive-" + LEX_CAST_STR(value) + " where `" + s_value.c_str()+ "`=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id)))
    {
        CHECK_DB_ERR(q);
        ret = HC_ERROR;
    }
    else
    {
        ret = HC_SUCCESS;
    }
    //只有不再需要为任何人保存才真的删除
    q.execute("delete from char_mails where archive='0'");
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "delMail") );
    robj.push_back( Pair("purpose", 1) );

    robj.push_back( Pair("s", ret) );
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return ret;
}

//删除邮件 0x3
int deleteMail(int cid, const std::string& cname, int purpose, json_spirit::mArray& idlist)
{
    int ret = 0, value = 2;
    std::string s_value = "";
    Query q(GetDb());
    if (purpose == 1)//收件箱
    {
        value = 2;
        s_value = "cid";
    }
    else if(purpose == 2)//发件箱
    {
        value = 1;
        s_value = "from";
    }
    //archive信件保存状态（1为发件人保存2为收件人保存3为双方保存）
    std::string sql = "update char_mails set archive=archive-" + LEX_CAST_STR(value) + " where `" + s_value.c_str()+ "`=" + LEX_CAST_STR(cid) + " and id in (0";

    try
    {
        for (json_spirit::mArray::iterator it = idlist.begin(); it != idlist.end(); ++it)
        {
            if ((*it).type() != json_spirit::obj_type)
            {
                continue;
            }
            json_spirit::mObject& tmp_obj = (*it).get_obj();
            int tmp_id = 0;
            READ_INT_FROM_MOBJ(tmp_id,tmp_obj,"id");
            sql += ("," + LEX_CAST_STR(tmp_id));
        }
        sql += ")";
    }
    catch(...)
    {
        return HC_ERROR;
    }
    if (!q.execute(sql))
    {
        CHECK_DB_ERR(q);
        ret = HC_ERROR;
    }
    else
    {
        ret = HC_SUCCESS;
    }
    //只有不再需要为任何人保存才真的删除
    q.execute("delete from char_mails where archive='0'");
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "delMail") );
    robj.push_back( Pair("purpose", 1) );

    robj.push_back( Pair("s", ret) );
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return ret;
}

//发送邮件 0x4
int sendMail(int cid, const std::string& cname, const std::string& to, const std::string& title, const std::string& content)
{
    int ret = HC_SUCCESS;
    Query q(GetDb());
    int to_id = 0;
    q.get_result("select id from charactors where name='" + GetDb().safestr(to) + "'");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        to_id = q.getval();
    }
    else
    {
        ret = HC_ERROR;
    }
    q.free_result();
    if (to_id > 0 && to_id != cid)
    {
        boost::shared_ptr<CharData> to_cd = GeneralDataMgr::getInstance()->GetCharData(to_id);
        if (!to_cd.get())
            return HC_ERROR;
        std::string sql = "";
        if (cid)
        {
            sql = "insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','3','2'," + LEX_CAST_STR(cid)
                + ",'" + GetDb().safestr(title)
                + "','" + GetDb().safestr(content)
                + "','" + LEX_CAST_STR(to_id) + "')";
        }
        else
        {
            sql = "insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','3','1'," + LEX_CAST_STR(cid)
                + ",'" + GetDb().safestr(title)
                + "','" + GetDb().safestr(content)
                + "','" + LEX_CAST_STR(to_id) + "')";
        }
        if (!q.execute(sql))
        {
            ret = HC_ERROR;
        }
        else
        {
            mailCmd cmd;
            cmd.mobj["name"] = to_cd->m_name;
            cmd.cid = to_id;
            cmd.cmd = mail_cmd_get_unread_list;
            InsertMailcmd(cmd);

            ret = HC_SUCCESS;
        }
        CHECK_DB_ERR(q);
        //cout<<"sql:"<<q.GetLastQuery()<<endl;
    }
    else
    {
        ret = HC_ERROR_SEND_MAIL_INVALID_DEST;
    }
    if (cid)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
        if (account.get())
        {
            json_spirit::Object robj;
            robj.push_back( Pair("cmd", "sendMail") );
            robj.push_back( Pair("s", ret) );
            std::string msg = getErrMsg(ret);
            if ("" != msg)
            {
                robj.push_back( Pair("msg", msg));
            }
            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
        }
    }
    return ret;
}

/*发送系统邮件
    event_type :
        0 普通信件
        1 好友护送邀请
        2 好友推送
        3
*/
int sendSystemMail(const std::string& name, int to_id, const std::string& title, const std::string& content, std::string attach, int battleId, int event_type, int event_extra)
{
    //cout<<"send system mail to "<<to_id<<endl;
    int ret = HC_SUCCESS;
    if (to_id > 0)
    {
        int attach_state = 0;
        if (attach != "")
            attach_state = 1;
        InsertSaveDb("insert into char_mails (input,unread,archive,type,`from`,title,content,attach,attach_state,cid,battleId,event_type,event_extra) values (now(),'1','2','1',0,'" + GetDb().safestr(title)
                + "','" + GetDb().safestr(content)
                + "','" + GetDb().safestr(attach)
                + "','" + LEX_CAST_STR(attach_state)
                + "','" + LEX_CAST_STR(to_id)
                + "','" + LEX_CAST_STR(battleId)
                + "','" + LEX_CAST_STR(event_type)
                + "','" + LEX_CAST_STR(event_extra)
                + "')", mail_cmd_get_unread_list, to_id, name, "");
    }
    else
    {
        ret = HC_ERROR_SEND_MAIL_INVALID_DEST;
    }
    return ret;
}

int getUnread(int cid, const std::string& cname)
{
    int counts = 0;
    Query q(GetDb());
    std::string sql = "select count(*) from char_mails where unread='1' and cid=" + LEX_CAST_STR(cid);
    q.get_result(sql);
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        counts = q.getval();
    }
    q.free_result();
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "getUnread") );
    robj.push_back( Pair("nums", counts) );
    robj.push_back( Pair("s", HC_SUCCESS) );
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

int getUnreadList(int cid, const std::string& cname)
{
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        json_spirit::Object robj;
        json_spirit::Array mlist;
        Query q(GetDb());
        std::string sql = "select id,input,title,content,type,battleId,event_type,event_extra,attach_state from char_mails where unread='1' and (archive='2' or archive='3') and cid=" + LEX_CAST_STR(cid) + " order by input desc limit 60";
        q.get_result(sql);
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int id = q.getval();
            json_spirit::Object info;
            info.push_back( Pair("id", id) );
            info.push_back( Pair("time", q.getstr()) );
            info.push_back( Pair("title", q.getstr()) );
            info.push_back( Pair("content", q.getstr()) );
            info.push_back( Pair("type", q.getval()) );
            info.push_back( Pair("battleId", q.getval()) );
            info.push_back( Pair("event_type", q.getval()) );
            info.push_back( Pair("event_extra", q.getval()) );
            info.push_back( Pair("attach_state", q.getval()) );
            mlist.push_back(info);
        }
        q.free_result();
        robj.push_back( Pair("cmd", "getUnreadList") );
        robj.push_back( Pair("list", mlist) );
        robj.push_back( Pair("s", HC_SUCCESS) );

        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return 0;
}

//领取邮件附件
int getMailAttach(int cid, const std::string& cname, int id)
{
    boost::shared_ptr<CharData> pc = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!pc.get())
    {
        return HC_ERROR;
    }
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "getMailAttach") );
    Query q(GetDb());
    std::string sql = "select attach,event_extra from char_mails where cid=" + LEX_CAST_STR(cid)
                            + " and id=" + LEX_CAST_STR(id) + " and attach_state='1'";
    q.get_result(sql);
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string attach = q.getstr();
        int statistics_type = q.getval();
        q.free_result();
        if (attach != "")
        {
            json_spirit::mValue value;
            json_spirit::read(attach, value);

            if (value.type() != json_spirit::array_type)
            {
                robj.push_back( Pair("s", HC_ERROR) );
            }
            else
            {
                std::list<Item> _items;
                json_spirit::mArray list = value.get_array();
                json_spirit::mArray::iterator it = list.begin();
                while (it != list.end())
                {
                    if ((*it).type() != json_spirit::obj_type)
                    {
                        ++it;
                        continue;
                    }
                    json_spirit::mObject& tmp_obj = (*it).get_obj();
                    int type = 0, id = 0, count = 0, extra = 0, extra2 = 0;
        			double fac_a = 0.0, fac_b = 0.0, fac_c = 0.0, fac_d = 0.0;
                    READ_INT_FROM_MOBJ(type,tmp_obj,"type");
                    READ_INT_FROM_MOBJ(id,tmp_obj,"id");
                    READ_INT_FROM_MOBJ(count,tmp_obj,"count");
                    READ_INT_FROM_MOBJ(extra,tmp_obj,"extra");
                    Item item(type, id, count, extra);
                    READ_INT_FROM_MOBJ(extra2,tmp_obj,"extra2");
					READ_REAL_FROM_MOBJ(fac_a,tmp_obj,"fac_a");
					READ_REAL_FROM_MOBJ(fac_b,tmp_obj,"fac_b");
					READ_REAL_FROM_MOBJ(fac_c,tmp_obj,"fac_c");
					READ_REAL_FROM_MOBJ(fac_d,tmp_obj,"fac_d");
                    item.extra2 = extra2;
                    item.d_extra[0] = fac_a;
                    item.d_extra[1] = fac_b;
                    item.d_extra[2] = fac_c;
                    item.d_extra[3] = fac_d;
                    //cout << "type = " << type << " id = " << id << " count = " << count << endl;
                    _items.push_back(item);
                    ++it;
                }
                //给东西
                std::list<Item> items = _items;
                if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
                {
                    robj.push_back( Pair("s", HC_ERROR_NOT_ENOUGH_BAG_SIZE) );
                    std::string msg = getErrMsg(HC_ERROR_NOT_ENOUGH_BAG_SIZE);
                    if ("" != msg)
                    {
                        robj.push_back( Pair("msg", msg));
                    }
                    return HC_ERROR;
                }
                else
                {
                    giveLoots(pc.get(), items, NULL, &robj, true, statistics_type);
                    robj.push_back( Pair("s", 200) );
                }
            }
        }
        else
        {
            robj.push_back( Pair("s", HC_ERROR) );
        }
        if (!q.execute("update char_mails set attach_state=2 where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id)))
        {
            CHECK_DB_ERR(q);
        }
    }
    else
    {
        robj.push_back( Pair("s", HC_ERROR_MAIL_ATTACH) );
        std::string msg = getErrMsg(HC_ERROR_MAIL_ATTACH);
        if ("" != msg)
        {
            robj.push_back( Pair("msg", msg));
        }
        q.free_result();
    }
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

//领取所有邮件附件
int getAllMailAttach(int cid, const std::string& cname)
{
    boost::shared_ptr<CharData> pc = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!pc.get())
    {
        return HC_ERROR;
    }
    bool err_break = false;
    bool has_attach = false;
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "getAllMailAttach") );
    Query q(GetDb());
    std::string sql = "select id,attach,event_extra from char_mails where cid=" + LEX_CAST_STR(cid)
                            + " and attach_state='1'";
    q.get_result(sql);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        has_attach = true;
        int id = q.getval();
        std::string attach = q.getstr();
        int statistics_type = q.getval();
        if (attach != "")
        {
            json_spirit::mValue value;
            json_spirit::read(attach, value);

            if (value.type() != json_spirit::array_type)
            {
                continue;
            }
            else
            {
                std::list<Item> _items;
                json_spirit::mArray list = value.get_array();
                json_spirit::mArray::iterator it = list.begin();
                while (it != list.end())
                {
                    if ((*it).type() != json_spirit::obj_type)
                    {
                        ++it;
                        continue;
                    }
                    json_spirit::mObject& tmp_obj = (*it).get_obj();
                    int type = 0, id = 0, count = 0, extra = 0, extra2 = 0;
        			double fac_a = 0.0, fac_b = 0.0, fac_c = 0.0, fac_d = 0.0;
                    READ_INT_FROM_MOBJ(type,tmp_obj,"type");
                    READ_INT_FROM_MOBJ(id,tmp_obj,"id");
                    READ_INT_FROM_MOBJ(count,tmp_obj,"count");
                    READ_INT_FROM_MOBJ(extra,tmp_obj,"extra");
                    Item item(type, id, count, extra);
                    READ_INT_FROM_MOBJ(extra2,tmp_obj,"extra2");
					READ_REAL_FROM_MOBJ(fac_a,tmp_obj,"fac_a");
					READ_REAL_FROM_MOBJ(fac_b,tmp_obj,"fac_b");
					READ_REAL_FROM_MOBJ(fac_c,tmp_obj,"fac_c");
					READ_REAL_FROM_MOBJ(fac_d,tmp_obj,"fac_d");
                    item.extra2 = extra2;
                    item.d_extra[0] = fac_a;
                    item.d_extra[1] = fac_b;
                    item.d_extra[2] = fac_c;
                    item.d_extra[3] = fac_d;
                    //cout << "type = " << type << " id = " << id << " count = " << count << endl;
                    _items.push_back(item);
                    ++it;
                }
                //给东西
                std::list<Item> items = _items;
                if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(items)))
                {
                    robj.push_back( Pair("s", HC_ERROR_NOT_ENOUGH_BAG_SIZE) );
                    err_break = true;
                    break;
                }
                else
                {
                    giveLoots(pc.get(), items, NULL, &robj, true, statistics_type);
                }
            }
        }
        else
        {
            continue;
        }
        InsertSaveDb("update char_mails set attach_state=2 where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id));
    }
    q.free_result();
    if (!has_attach)
    {
        robj.push_back( Pair("s", HC_ERROR_MAIL_ATTACH) );
    }
    else if (!err_break)
    {
        robj.push_back( Pair("s", 200) );
    }
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

mailCmd::mailCmd()
{
    cid = 0;
    cmd = 0;
}

//异步读取邮件
bool mailProcesser::work(mailCmd &mCmd)       // 在些完成实际任务.
{
    try
    {
        //处理命令
        switch (mCmd.cmd)
        {
            case mail_cmd_get_list:
                {
                    int page = 0, pageNum = 0, purpose = 0;
                    READ_INT_FROM_MOBJ(purpose,mCmd.mobj,"purpose");
                    READ_INT_FROM_MOBJ(page,mCmd.mobj,"page");
                    READ_INT_FROM_MOBJ(pageNum,mCmd.mobj,"pageNums");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getMaillist(mCmd.cid, cname, purpose, page, pageNum);
                }
                break;
            case mail_cmd_query_mail:
                {
                    int id = 0, purpose = 0;
                    READ_INT_FROM_MOBJ(id,mCmd.mobj,"id");
                    READ_INT_FROM_MOBJ(purpose,mCmd.mobj,"purpose");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    queryMailContent(mCmd.cid, cname, purpose, id);
                }
                break;
            case mail_cmd_delete_mail:
                {
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    int id = 0, purpose = 0;
                    READ_INT_FROM_MOBJ(id,mCmd.mobj,"id");
                    READ_INT_FROM_MOBJ(purpose,mCmd.mobj,"purpose");
                    if (id == 0)
                    {
                        json_spirit::mArray idlist;
                        READ_ARRAY_FROM_MOBJ(idlist, mCmd.mobj, "idlist");
                        if (idlist.size() > 0)
                        {
                            deleteMail(mCmd.cid, cname, purpose, idlist);
                        }
                    }
                    else
                    {
                        deleteMail(mCmd.cid, cname, purpose, id);
                    }
                }
                break;
            case mail_cmd_send_mail:
                {
                    std::string to = "", title = "", content = "", cname = "";
                    READ_STR_FROM_MOBJ(to, mCmd.mobj,"to");
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    READ_STR_FROM_MOBJ(title, mCmd.mobj,"title");
                    READ_STR_FROM_MOBJ(content, mCmd.mobj,"content");
                    //关键字过滤
                    Forbid_word_replace::getInstance()->Filter(content);
                    sendMail(mCmd.cid, cname, to, title, content);
                }
                break;
            case mail_cmd_get_unread:
                {
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getUnread(mCmd.cid, cname);
                }
                break;
            case mail_cmd_get_unread_list:
                {
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getUnreadList(mCmd.cid, cname);
                }
                break;
            case mail_cmd_keep_db:
                {
                    //保持数据库连接
                    json_spirit::mObject o;
                    ProcessKeepDb(o);
                }
                break;
            case mail_cmd_get_mail_attach:
                {
                    int id = 0;
                    READ_INT_FROM_MOBJ(id,mCmd.mobj,"id");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getMailAttach(mCmd.cid, cname, id);
                }
                break;
            case mail_cmd_get_all_mail_attach:
                {
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getAllMailAttach(mCmd.cid, cname);
                }
                break;
        }
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "mailProcesser work , Exception: " << e.what() << "\n";
    }
    return true;
}

