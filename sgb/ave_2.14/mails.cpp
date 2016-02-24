
#include "mails.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_all.h"
#include "json_spirit.h"
#include "spls_errcode.h"
#include "data.h"
#include "combat.h"
#include "statistics.h"
#include "text_filter.h"

extern int ProcessKeepDb(json_spirit::mObject& o);
Database& GetDb();
extern void InsertSaveDb(const std::string& sql, int param1, int param2, const std::string& sp1, const std::string& sp2);

extern void InsertMailcmd(mailCmd&);
extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

//读取邮件列表 0x1
int getMaillist(int cid,  const std::string& cname, int type, int page, int pageOfNums)
{
    bool archive = false;
    if (type == 3)
    {
        archive = true;
        type = 0;
    }
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
    std::string sql = "select cm.id,cm.input,cm.title,c.name,cm.unread,cm.type,attach_state from char_mails as cm left join charactors as c on cm.`from`=c.id where cm.cid=" + LEX_CAST_STR(cid);
    if (type != 0)
    {
        sql += " and type=" + LEX_CAST_STR(type);
    }
    if (archive)
    {
        sql += " and archive='1'";
    }
    else
    {
        sql += " and archive='0'";
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
            obj.push_back( Pair("sender", q.getstr()) );
            obj.push_back( Pair("unread", q.getval()) );
            obj.push_back( Pair("type", q.getval()) );
            obj.push_back( Pair("attach_state", q.getval()) );
            mlist.push_back(obj);
        }
    }
    q.free_result();

    robj.push_back( Pair("cmd", "getMailList") );
    robj.push_back( Pair("s", 200) );
    robj.push_back( Pair("type", type) );

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
int queryMailContent(int cid, const std::string& cname, int id)
{
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "queryMail") );
    Query q(GetDb());
    std::string sql = "select cm.id,cm.input,cm.title,c.name,cm.content,cm.type,cm.attach,cm.attach_state from char_mails as cm left join charactors as c on cm.`from`=c.id where cm.cid=" + LEX_CAST_STR(cid)
                            + " and cm.id=" + LEX_CAST_STR(id);
    q.get_result(sql);
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        json_spirit::Object info;
        robj.push_back( Pair("s", 200) );
        info.push_back( Pair("id", q.getval()) );
        info.push_back( Pair("time", q.getstr()) );
        info.push_back( Pair("title", q.getstr()) );
        info.push_back( Pair("sender", q.getstr()) );
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
                    int type = 0, id = 0, count = 0, fac = 0;
                    READ_INT_FROM_MOBJ(type,tmp_obj,"type");
                    READ_INT_FROM_MOBJ(id,tmp_obj,"id");
                    READ_INT_FROM_MOBJ(count,tmp_obj,"count");
                    READ_INT_FROM_MOBJ(fac,tmp_obj,"fac");
                    Item item;
                    item.type = type;
                    item.id = id;
                    item.nums = count;
                    item.fac = fac;
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
        if (!q.execute("update char_mails set unread=0 where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id)))
        {
            CHECK_DB_ERR(q);
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
int deleteMail(int cid, const std::string& cname, int id)
{
    int ret = 0;
    Query q(GetDb());
    if (!q.execute("delete from char_mails where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id)))
    {
        CHECK_DB_ERR(q);
        ret = HC_ERROR;
    }
    else
    {
        ret = HC_SUCCESS;
    }
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "setMailAction") );
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
int deleteMail(int cid, const std::string& cname, json_spirit::mArray& idlist)
{
    int ret = 0;
    Query q(GetDb());
    std::string sql = "delete from char_mails where cid=" + LEX_CAST_STR(cid) + " and id in (0";

    try
    {
        for (json_spirit::mArray::iterator it = idlist.begin(); it != idlist.end(); ++it)
        {
            int id = it->get_int();
            sql += LEX_CAST_STR(id);
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
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "setMailAction") );
    robj.push_back( Pair("purpose", 1) );

    robj.push_back( Pair("s", ret) );
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return ret;
}

//删除正页邮件 0x4
int deletePageOfMail(int cid, const std::string& cname, int type, int id, int nums)
{
    //cout<<"delete page of mail "<<cid<<","<<type<<","<<id<<","<<nums<<endl;
    bool archive = false;
    if (type == 5)
    {
        archive = true;
        type = 0;
    }
    Query q(GetDb());
    std::string sql = "select cm.id from char_mails as cm where cm.cid=" + LEX_CAST_STR(cid);
    if (type != 0)
    {
        sql += " and type=" + LEX_CAST_STR(type);
    }
    if (archive)
    {
        sql += " and archive='1'";
    }
    else
    {
        sql += " and archive='0'";
    }
    sql += " order by unread desc,input desc";

    int cur_nums = 0;
    std::list<int> mlist;
    //cout<<sql<<endl;
    q.get_result(sql);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mid = q.getval();
        if (mid == id)
        {
            cur_nums = 1;
        }
        else if (cur_nums > 0)
        {
            ++cur_nums;
        }
        if (cur_nums > 0)
        {
            mlist.push_back(mid);
        }
        if (cur_nums >= nums)
        {
            break;
        }
    }
    q.free_result();

    std::list<int>::iterator it = mlist.begin();
    while (it != mlist.end())
    {
        if (!q.execute("delete from char_mails where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(*it)))
        {
            CHECK_DB_ERR(q);
        }
        ++it;
    }
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "delAllMail") );
    robj.push_back( Pair("s", 200) );
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }     
    return HC_SUCCESS;
}

//保存邮件 0x5
int ArchiveMail(int cid, const std::string& cname, int id)
{
    int ret = 0;
    Query q(GetDb());
    q.get_result("select count(*) from char_mails where cid=" + LEX_CAST_STR(cid) + " and archive='1'");
    if (q.fetch_row())
    {
        int count = q.getval();
        if (count >= 10)
        {
            return HC_ERROR;
        }
    }
    q.free_result();

    if (!q.execute("update char_mails set archive='1' where cid=" + LEX_CAST_STR(cid) + " and id=" + LEX_CAST_STR(id)))
    {
        CHECK_DB_ERR(q);
        ret = HC_ERROR;
    }
    else
    {
        ret = HC_SUCCESS;
    }
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "setMailAction") );
    robj.push_back( Pair("purpose", 2) );
    robj.push_back( Pair("s", ret) );
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }     
    return ret;
}

//发送邮件 0x6
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
    if (to_id > 0)
    {
        std::string sql = "";
        if (cid)
        {
            sql = "insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','0','2'," + LEX_CAST_STR(cid)
                + ",'" + GetDb().safestr(title)
                + "','" + GetDb().safestr(content)
                + "','" + LEX_CAST_STR(to_id) + "')";
        }
        else
        {
            sql = "insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','0','1'," + LEX_CAST_STR(cid)
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
        InsertSaveDb("insert into char_mails (input,unread,archive,type,`from`,title,content,attach,attach_state,cid,battleId,event_type,event_extra) values (now(),'1','0','1',0,'" + GetDb().safestr(title)
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
        std::string sql = "select id,input,title,content,type,battleId,event_type,event_extra,attach_state from char_mails where unread='1' and archive='0' and cid=" + LEX_CAST_STR(cid) + " order by input desc limit 60";
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

//获取GM信件列表
int getGMList(int cid, const std::string& cname)
{
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "getGMqlist") );
    json_spirit::Array list;
    Query q(GetDb());
    int min_save_qid = 0;
    q.get_result("SELECT id,question,answer FROM `char_gm_question` WHERE cid=" + LEX_CAST_STR(cid) + " order by input desc limit 10");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int qid = q.getval();
        std::string question = q.getstr();
        std::string answer = q.getstr();
        json_spirit::Object obj;
        obj.push_back( Pair("question", question) );
        obj.push_back( Pair("answer", answer) );
        list.push_back(obj);
        min_save_qid = qid;
    }
    q.free_result();
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("s", HC_SUCCESS) );
    q.execute("delete from char_gm_question where cid = " + LEX_CAST_STR(cid) + " and id < " + LEX_CAST_STR(min_save_qid));
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
    if (account.get())
    {
        account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
    }
    return HC_SUCCESS;
}

//提交GM问题
int sendGMquestion(int cid, const std::string& cname, const std::string& content, int type)
{
    int ret = HC_SUCCESS;
    Query q(GetDb());
    if(q.execute("insert into char_gm_question (cid,type,question,answer,state,input) values ("
        + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(type) + ",'"
        + GetDb().safestr(content) + "','','0',now())") == false)
    {
        ret = HC_ERROR;
    }
    CHECK_DB_ERR(q);
    q.free_result();
    if (cid)
    {
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cname);
        if (account.get())
        {
            json_spirit::Object robj;
            robj.push_back( Pair("cmd", "subGMquestion") );
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
    std::string sql = "select cm.attach from char_mails as cm left join charactors as c on cm.`from`=c.id where cm.cid=" + LEX_CAST_STR(cid)
                            + " and cm.id=" + LEX_CAST_STR(id) + " and cm.attach_state='1'";
    q.get_result(sql);
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string attach = q.getstr();
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
                    int type = 0, id = 0, count = 0, fac = 0;
                    READ_INT_FROM_MOBJ(type,tmp_obj,"type");
                    READ_INT_FROM_MOBJ(id,tmp_obj,"id");
                    READ_INT_FROM_MOBJ(count,tmp_obj,"count");
                    READ_INT_FROM_MOBJ(fac,tmp_obj,"fac");
                    Item item;
                    item.type = type;
                    item.id = id;
                    item.nums = count;
                    item.fac = fac;

                    //cout << "type = " << type << " id = " << id << " count = " << count << endl;
                    _items.push_back(item);
                    ++it;
                }
                //给东西
                std::list<Item> items = _items;
                giveLoots(pc.get(), items, pc->m_area, pc->m_level, 0, NULL, &robj, true, give_mail_loot);
                robj.push_back( Pair("s", 200) );
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
                    int page = 0, pageNum = 0, type = 0;
                    READ_INT_FROM_MOBJ(type,mCmd.mobj,"type");
                    READ_INT_FROM_MOBJ(page,mCmd.mobj,"page");
                    READ_INT_FROM_MOBJ(pageNum,mCmd.mobj,"pageNums");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getMaillist(mCmd.cid, cname, type, page, pageNum);
                }
                break;
            case mail_cmd_query_mail:
                {
                    int id = 0;
                    READ_INT_FROM_MOBJ(id,mCmd.mobj,"id");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    queryMailContent(mCmd.cid, cname, id);
                }
                break;
            case mail_cmd_delete_mail:
                {
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    int id = 0;
                    READ_INT_FROM_MOBJ(id,mCmd.mobj,"id");
                    if (id == 0)
                    {
                        json_spirit::mArray idlist;
                        READ_ARRAY_FROM_MOBJ(idlist, mCmd.mobj, "idlist");
                        if (idlist.size() > 0)
                        {
                            deleteMail(mCmd.cid, cname, idlist);
                        }
                    }
                    else
                    {
                        deleteMail(mCmd.cid, cname, id);
                    }
                }
                break;
            case mail_cmd_delete_mails:
                {
                    int type = 0, id = 0, nums = 0;
                    READ_INT_FROM_MOBJ(type, mCmd.mobj,"type");
                    READ_INT_FROM_MOBJ(id, mCmd.mobj,"id");
                    READ_INT_FROM_MOBJ(nums, mCmd.mobj,"nums");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    deletePageOfMail(mCmd.cid, cname, type, id, nums);
                }
                break;
            case mail_cmd_archive_mail:
                {
                    int id = 0;
                    READ_INT_FROM_MOBJ(id, mCmd.mobj,"id");
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    ArchiveMail(mCmd.cid, cname, id);
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
            case mail_cmd_set_unread:
                {
                    Query q(GetDb());
                    int id = 0;
                    READ_INT_FROM_MOBJ(id, mCmd.mobj,"id");
                    if (0 == id)
                    {
                        q.execute("delete from char_mails where cid=" + LEX_CAST_STR(mCmd.cid) + " and unread='1' and attach_state!='1'");
                        q.execute("update char_mails set unread='0' where cid=" + LEX_CAST_STR(mCmd.cid) + " and unread='1'");
                    }
                    else
                    {
                        q.execute("update char_mails set unread='0' where cid=" + LEX_CAST_STR(mCmd.cid) + " and id=" + LEX_CAST_STR(id));
                        q.execute("delete from char_mails where cid=" + LEX_CAST_STR(mCmd.cid) + " and id=" + LEX_CAST_STR(id) + " and attach_state!='1'");
                    }
                    CHECK_DB_ERR(q);
                }
                break;
            case mail_cmd_get_gm_question:
                {
                    std::string cname = "";
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    getGMList(mCmd.cid, cname);
                }
                break;
            case mail_cmd_send_gm_question:
                {
                    std::string content = "", cname = "";
                    int type = 0;
                    READ_STR_FROM_MOBJ(cname, mCmd.mobj,"name");
                    READ_STR_FROM_MOBJ(content, mCmd.mobj,"question");
                    READ_INT_FROM_MOBJ(type, mCmd.mobj,"type");
                    sendGMquestion(mCmd.cid, cname, content, type);
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
        }
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "mailProcesser work , Exception: " << e.what() << "\n";
    }
    return true;
}

