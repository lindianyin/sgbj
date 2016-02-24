
#include "db_thread.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_all.h"
#include "json_spirit.h"
#include "spls_errcode.h"
#include "data.h"
using namespace net;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
//保持数据库连接
extern int ProcessKeepDb(json_spirit::mObject& o);

extern int InsertInternalActionWork(json_spirit::mObject& obj);

int loadCharFromDb(int cid)
{
    //cout<<"loadCharFromDb--> new "<<cid<<",tid:"<<syscall(SYS_gettid)<<endl;
    //boost::shared_ptr<OnlineUser> paccount = GeneralDataMgr::getInstance()->GetAccount(account);
    CharData* pc = new CharData(cid);
    json_spirit::mObject obj;
    obj["cmd"] = "ProcessLoadCharCallback";
    obj["cid"] = cid;
    obj["data"] = (boost::uint64_t)pc;
    if (0 != InsertInternalActionWork(obj))
    {
        ERR();
    }
    return HC_SUCCESS;
}

int loadAccountFromDb(const std::string& account)
{
    //cout<<"##################### loadAccountFromDb "<<account<<" ###########################"<<endl;
    //获得角色登录列表
    CharactorInfo** pCharactorlist = NULL;
    int counts = 0;

    Query q(GetDb());
#ifdef ONE_CHARACTOR
    q.get_result("SELECT c.id,c.lastlogin,c.spic,c.state,c.delete_time,c.level,c.name FROM `charactors` as c left join char_data as cd on c.id=cd.cid WHERE c.account='" + GetDb().safestr(account) + "' order by id limit 1");
#else
    q.get_result("SELECT c.id,c.lastlogin,c.spic,c.state,c.delete_time,c.level,c.name FROM `charactors` as c left join char_data as cd on c.id=cd.cid WHERE c.account='" + GetDb().safestr(account) + "' order by lastlogin desc");
#endif
    CHECK_DB_ERR(q);
    if (q.num_rows())
    {
        counts = q.num_rows();
        //cout<<"##################### loadAccountFromDb "<<counts<<" ###########################"<<endl;

        pCharactorlist = new CharactorInfo*[counts];
        int index = 0;
        while (q.fetch_row())
        {            
            //cout<<"##################### loadAccountFromDb ??? ###########################"<<endl;
            pCharactorlist[index] = new CharactorInfo;
            pCharactorlist[index]->m_cid = q.getval();
            pCharactorlist[index]->m_lastlogin = q.getval();
            pCharactorlist[index]->m_spic = q.getval();
            pCharactorlist[index]->m_state = q.getval();
            pCharactorlist[index]->m_deleteTime = q.getval();
            pCharactorlist[index]->m_level = q.getval();
            //cout<<"##################### loadAccountFromDb !!! ###########################"<<endl;
            pCharactorlist[index]->m_name = q.getstr();
            ++index;
        }
    }
    q.free_result();
    //cout<<"##################### loadAccountFromDb done ###########################"<<endl;

    json_spirit::mObject obj;
    obj["cmd"] = "charlist";
    obj["account"] = account;
    obj["clist"] = (boost::uint64_t)pCharactorlist;
    obj["size"] = counts;
    if (0 != InsertInternalActionWork(obj))
    {
        ERR();
    }
    return HC_SUCCESS;
}

int queryAccountScore(const std::string& account)
{
    Query q(GetDb());
    q.get_result("select money from accounts where account='" + GetDb().safestr(account) + "'");
    CHECK_DB_ERR(q);
    if (!q.fetch_row())
    {
        q.free_result();
        return 0;
    }
    int money = q.getval();
    q.free_result();
    return money;
}

//异步读取数据库的线程
bool charDbProcesser::work(dbCmd &mCmd)       // 在些完成实际任务.
{
    try
    {
        //处理命令
        switch (mCmd._cmd)
        {
            case db_cmd_load_account:
                loadAccountFromDb(mCmd._account);
                break;
                
            case db_cmd_load_char:
                loadCharFromDb(mCmd._cid);
                break;

            case db_cmd_keep_db:
                //保持数据库连接
                {
                    json_spirit::mObject o;
                    ProcessKeepDb(o);
                }
                break;
            case db_cmd_load_char_gag:
                //查询角色是否被禁言
                {
                    Query q(GetDb());
                    q.get_result("select endtime from admin_speak where endtime>unix_timestamp() and cid=" + LEX_CAST_STR(mCmd._cid));
                    CHECK_DB_ERR(q);
                    if (q.fetch_row())
                    {
                        time_t endtime = q.getuval();
                        q.free_result();
                        if (endtime > 0)
                        {
                            //通知禁言
                            json_spirit::mObject obj;
                            obj["cmd"] = "scheduleEvent";
                            obj["event"] = "gagChar";
                            obj["param1"] = mCmd._cid;
                            obj["param2"] = endtime;
                            if (0 != InsertInternalActionWork(obj))
                            {
                                ERR();
                            }
                        }
                    }
                    else
                    {
                        q.free_result();
                    }
                    break;
                }
            case db_cmd_query_account_score:
                {
                    //查询帐号点数                    
                    int money = queryAccountScore(mCmd._account);
                    json_spirit::mObject obj;
                    obj["cmd"] = "queryScore";
                    obj["account"] = mCmd._account;
                    obj["score"] = money;
                    if (0 != InsertInternalActionWork(obj))
                    {
                        ERR();
                    }
                    break;
                }
            case db_cmd_char_online:
                {
                    //更细统计系统中的登录时间和上线状态
                    Query q(GetDb());
                    q.get_result("select count(*) from admin_char where cid=" + LEX_CAST_STR(mCmd._cid));
                    CHECK_DB_ERR(q);
                    if (q.fetch_row())
                    {
                        time_t now = time(NULL);
                        if (q.getval() > 0)
                        {
                            q.free_result();
                            q.execute("update admin_char set cstat='1',last_login=" + LEX_CAST_STR(now) + " where cid=" + LEX_CAST_STR(mCmd._cid));
                        }
                        else
                        {
                            q.free_result();
                            q.execute("insert into admin_char set cstat='1',ctime=0,tnum=1,last_login=" + LEX_CAST_STR(now) + ",cid=" + LEX_CAST_STR(mCmd._cid));
                            CHECK_DB_ERR(q);
                        }
                    }
                    else
                    {
                        q.free_result();
                    }                    
                    break;
                }
                break;
            case db_cmd_char_offline:
                {
                    Query q(GetDb());
                    //更细统计系统中的总上线时间和上线状态
                    q.execute("update admin_char set ctime=if(unix_timestamp()>last_login,ctime+unix_timestamp()-last_login,ctime),cstat='0' where cid=" + LEX_CAST_STR(mCmd._cid));
                    CHECK_DB_ERR(q);
                    break;
                }
                break;
        }
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "charDbProcesser work , Exception: " << e.what() << "\n";
    }
    return true;
}

