#pragma once

#include "login.h"
#include "utils_all.h"
#include "net.h"
#include "worker.hpp"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include <sys/syscall.h>

using namespace net;

extern Database& GetDb();

extern volatile int g_auth_type;    // 0:md5 , 1:ticket

extern int ProcessKeepDb(json_spirit::mObject& o);
int InsertActionWork(actionmessage& msg);

//测试用户验证
extern int AuthAccount(const std::string& account, const std::string& password);

//平台用户验证
extern int platformAuthAccount(const std::string& qid,
                          const std::string& qname,
                          int union_id,
                          const std::string& server_id,
                          time_t _time,
                          int iAdult,
                          const std::string& extra1,
                          const std::string& extra2,
                          const std::string& sign,
                          std::string& account);


extern int ticketAuth( const std::string& ip,
                        const std::string& ticket, std::string& qid,
                          std::string& qname,
                          int& union_id,
                          std::string& server_id,
                          int& iAdult,
                          std::string& extra1,
                          std::string& extra2,                          
                          std::string& account);

#ifdef QQ_PLAT
struct admin_qq_yellow_data
{
    int year_yellow;
    int yellow_level;
};
#endif

class LoginWorker : public worker<loginmessage>
{
public:
    LoginWorker(jobqueue<loginmessage>& _jobqueue, std::size_t _maxthreads = 1)
    :worker<loginmessage>("login",_jobqueue, _maxthreads)
    {
#ifdef QQ_PLAT
        Query q(GetDb());
        q.get_result("select account,year_yellow,yellow_level from char_qq_yellow where 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            admin_qq_yellow_data ad;
            std::string account = q.getstr();
            ad.year_yellow = q.getval();
            ad.yellow_level = q.getval();
            m_admin_qq_yellow_datas[account] = ad;
        }
        q.free_result();
#endif
    }
    virtual bool work(loginmessage& task)       // 在些完成实际任务.
    {
        int ret = 0;
        try
        {
            session_ptr psession;
            task.getsession(psession);
            if (!psession.get()) 
            {
                if (task.username == "keepDb" && task.password == "")
                {
                    //保持数据库连接
                    json_spirit::mObject o;
                    ProcessKeepDb(o);
                    return true;
                }
                INFO("login worker get no session"<<endl);
                return true;
            }
            using namespace std;
            using namespace boost;
            using namespace json_spirit;

            //cout<<"############# auth login cmd "<<splsTimeStamp()<<",user:"<<task.username<<",qid:"<<task._qid<<endl;
            
            if (psession->state() == STATE_CONNECTED)
            {
                std::string account = task.username;
                if (account != "")
                {
                    if (psession->remote_ip() == "127.0.0.1")
                    {
                        if (account == "robot")
                        {
                            Query q(GetDb());
                            q.get_result("select uuid()");
                            task.username = q.getstr();
                            q.free_result();
                            account = task.username;                            
                        }
                        else
                        {

                        }
                        task._qid = "";
                        task._qname = "";
                        task._server_id = "";
                        task._isAdult = 1;
                        ret = HC_SUCCESS;
                    }
                    else
                    {
                        //验证用户
                        ret = AuthAccount(task.username, task.password);
                        task._isAdult = 1;
                        task._qid = account;
                    }
                }
                else
                {
                    if (g_auth_type == 0)
                    {
                        //平台用户验证
                        ret = platformAuthAccount(task._qid, task._qname,
                                        task._union_id, task._server_id,
                                        task._time, task._isAdult,
                                           task._extra1, task._extra2,
                                           task._sign, account);
                    }
                    else
                    {
                        ret = ticketAuth(psession->remote_ip(),
                                          task._sign,
                                          task._qid,
                                          task._qname,
                                          task._union_id,
                                          task._server_id,
                                          task._isAdult,
                                          task._extra1,
                                          task._extra2,
                                          account);
                    }
                }
                switch (ret)
                {
                    case HC_SUCCESS://成功
                        //login success
                        psession->state(STATE_AUTHED);
                        psession->retry(0);                        
                        break;
                
                    case HC_ERROR_WRONG_ACCOUNT://用户名不存在
                    case HC_ERROR_WRONG_PASSWORD://密码错误
                    default:
                        {
                            Object robj;
                            robj.clear();
                            robj.push_back( Pair("cmd", "bye"));
                            robj.push_back( Pair("s", ret));
                            std::string msg = getErrMsg(ret);
                            if ("" != msg)
                            {
                                robj.push_back( Pair("msg", msg));
                            }
                            psession->send(write(robj, json_spirit::raw_utf8));
                            //quit
                            psession->closeconnect();
                            return true;
                        }
                }

                //验证通过,登录
                json_spirit::mObject obj;
                obj["cmd"] = "login";
                obj["user"] = account;
                obj["qid"] = task._qid;
                obj["union_id"] = task._union_id;
                obj["server_id"] = task._server_id;
                obj["isAdult"] = task._isAdult;
#ifdef QQ_PLAT
                if (task._is_qq_yellow && task._qq_yellow_level > 0)
                {
                    obj["isYearYellow"] = task._is_qq_year_yellow;
                    obj["qqYellowLevel"] = task._qq_yellow_level;
                }
                else if (m_admin_qq_yellow_datas.find(account) != m_admin_qq_yellow_datas.end())
                {
                    obj["isYearYellow"] = m_admin_qq_yellow_datas[account].year_yellow;
                    obj["qqYellowLevel"] = m_admin_qq_yellow_datas[account].yellow_level;
                }
                obj["iopenid"] = task._iopenid;
                obj["feedid"] = task._feedid;
                obj["login_str1"] = task._login_str1;
                obj["login_str2"] = task._login_str2;
#endif
                actionmessage act_msg(obj, 0);
                act_msg.setsession(psession);
                if (0 != InsertActionWork(act_msg))
                {
                    ERR();
                }
            }
            return true;
        }
        catch (std::exception& e)
        {
            syslog(LOG_ERR, "action work , Exception: %s", e.what());
            syslog(LOG_ERR, "cmd: login, ret: %d", ret);
            void * array[25];
            int nSize = backtrace(array, 25);
            char ** symbols = backtrace_symbols(array, nSize);
            for (int i = 0; i < nSize; i++)
            {
                syslog(LOG_ERR, symbols[i]);
            }
            free(symbols);
        }
        return true;
    }
private:
#ifdef QQ_PLAT
    std::map<std::string, admin_qq_yellow_data> m_admin_qq_yellow_datas;
#endif
};

