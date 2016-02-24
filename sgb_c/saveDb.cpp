
#include "SaveDb.h"
#include "mails.h"
uint64_t splsTimeStamp();

int ProcessKeepDb(json_spirit::mObject& o);

void InsertMailcmd(mailCmd& pmailCmd);

bool SavedbProcess::work(saveDbJob& job)        // 在些完成实际任务.
{
    try
    {
        std::list<uint64_t> times;
        if (g_print_debug_info == 1)
        {
            times.push_back(splsTimeStamp());
        }
        if (0 == job.sqls.size())
        {
            //保持数据库连接
            json_spirit::mObject o;
            ProcessKeepDb(o);
            return true;
        }
        Query q(GetDb());
        if (g_print_debug_info == 1)
        {
            times.push_back(splsTimeStamp());
        }
        while (job.sqls.size() > 0)
        {
            std::string sql = job.sqls.front();
            q.execute(sql);
            CHECK_DB_ERR(q);
            job.sqls.pop_front();
            if (g_print_debug_info == 1)
            {
                times.push_back(splsTimeStamp());
                printf("%s\n", sql.c_str());
            }
        }

        if (g_print_debug_info == 1)
        {
            printf("save db times ");
            for (std::list<uint64_t>::iterator it = times.begin(); it != times.end(); ++it)
            {
                printf("%ld,", *it);
            }
            printf("\n");
        }

        switch (job.param[0])
        {
            case mail_cmd_get_unread_list:
                {
                    mailCmd cmd;
                    cmd.mobj["name"] = job.strParam[0];
                    cmd.cid = job.param[1];
                    cmd.cmd = job.param[0];//mail_cmd_get_unread_list;
                    InsertMailcmd(cmd);
                }
                break;
        }
        return true;
    }
    catch (std::exception& e)
    {
        std::cerr << "SaveDb work , Exception: " << e.what() << "\n";
    }
    return true;
}

