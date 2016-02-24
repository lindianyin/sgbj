
/* 合服 */

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include <string>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "boost/date_time/gregorian/gregorian.hpp"

using namespace std;

#define CHECK_DB_ERR(q) if (q.GetErrno())\
    {\
         cout<<"sql:"<<q.GetLastQuery()<<endl<<"error:"<<q.GetError()<<",errno:"<<q.GetErrno()<<endl;\
    }

#define LEX_CAST_STR(x) boost::lexical_cast<std::string>(x)

uint64_t printTimeElapsed(bool p = true, uint64_t start_time = 0)
{
    static uint64_t stamp_now = 0;
    struct timeval _tstart;
    gettimeofday(&_tstart, NULL);
    uint64_t xstamp_now = 1000*1000*_tstart.tv_sec + _tstart.tv_usec;
    if (p)
    {
        uint64_t elasped = 0;
        if (start_time)
        {
            elasped = xstamp_now - start_time;
        }
        else
        {
            elasped = xstamp_now - stamp_now;
        }
        int secs = elasped / 1000000;
        if (secs)
        {
            cout<<secs<<" seconds, ";
        }
        cout<<(double((xstamp_now - stamp_now)%1000000)/1000)<<" ms"<<endl;
    }
    stamp_now = xstamp_now;
    return stamp_now;
}

/*

导出

/usr/local/mysql/bin/mysqldump -u c_user -p23rf234 shhx > shhx.backup

导入
/usr/local/mysql/bin/mysql -u c_user -p23rf234 shhx < shhx.backup

update test1.charactors set test1.charactors.id=test1.charactors.id+(select max(test2.charactors.id) from test2.charactors where 1) WHERE 1

update test1.charactors left join test2.charactors on test1.charactors.name=test2.charactors.name set test1.charactors.name=CONCAT(test1.charactors.name,".h1") WHERE test2.charactors.name!=''

INSERT INTO  `test2`.`charactors`
SELECT *
FROM  `test1`.`charactors` ;

INSERT INTO  `test2`.`char_xx`
SELECT *
FROM  `test1`.`char_xx` ;

*/

enum db_error
{
    db_error_table_not_exit = 1146,
};

struct base_pack_item
{
    int pid;
    int type;
    int id;
    int count;
};

struct base_pack
{
    int pid;
    int need_vip;
    int need_prestige;
    int need_level;
    int need_code;
    std::string name;
    std::string memo;

    std::list<base_pack_item> itemList;

    base_pack()
    {
        pid = 0;
        need_vip = 0;
        need_prestige = 0;
        need_level = 0;
        need_code = 0;
        name = "";
        memo = "";
    }
};

struct cpack
{
    int id;

    int enable;
    int openYear;
    int openMonth;
    int openDay;
    int openHour;
    int openMinute;
    int closeYear;
    int closeMonth;
    int closeDay;
    int closeHour;
    int closeMinute;

    std::string base_name;
    std::string name;
    std::string memo;
};

int loadBasePacks(Query& q, const std::string& db_name, std::map<std::string, base_pack>& packs)
{
    //礼包领取记录的处理，礼包id处理
    q.get_result("select name,id,vip,prestige,level,code,memo from " + db_name + ".custom_base_packs where 1");
    while (q.fetch_row())
    {
        std::string name = q.getstr();
        int pid = q.getval();
        if (packs.find(name) != packs.end())
        {
            cout<<"error, db "<<db_name<<".custom_base_packs name:"<<name<<endl;
            return -1;
        }
        base_pack bp;
        bp.name = name;
        bp.pid = pid;
        bp.need_vip = q.getval();
        bp.need_prestige = q.getval();
        bp.need_code = q.getval();
        bp.memo = q.getstr();
        packs[name] = bp;
    }
    q.free_result();

    for (std::map<std::string, base_pack>::iterator it = packs.begin(); it != packs.end(); ++it)
    {
        q.get_result("select type,id,count from " + db_name + ".custom_base_packs_items where pid=" + LEX_CAST_STR(it->second.pid));
        while (q.fetch_row())
        {
            base_pack_item bitem;
            bitem.type = q.getval();
            bitem.id = q.getval();
            bitem.count = q.getval();
            it->second.itemList.push_back(bitem);
        }
        q.free_result();
    }
    return 0;
}

int loadPacks(Query& q, const std::string& db_name, std::map<std::string, cpack>& packs, std::map<std::string, base_pack>& bpacks)
{
    //礼包领取记录的处理，礼包id处理
    q.get_result("select c.name,c.id,b.name,c.enable,c.openYear,c.openMonth,c.openDay,c.openHour,c.openMinute,c.closeYear,c.closeMonth,c.closeDay,c.closeHour,c.closeMinute,c.memo from " + db_name + ".custom_packs as c left join " + db_name + ".custom_base_packs as b on c.pid=b.id where 1");
    while (q.fetch_row())
    {
        std::string name = q.getstr();
        int id = q.getval();
        if (packs.find(name) != packs.end())
        {
            cout<<"error, db "<<db_name<<".custom_packs name:"<<name<<endl;
            return -1;
        }
        cpack bp;
        bp.name = name;
        bp.id = id;
        bp.base_name = q.getstr();
        bp.enable = q.getval();
        bp.openYear = q.getval();
        bp.openMonth = q.getval();
        bp.openDay = q.getval();
        bp.openHour = q.getval();
        bp.openMinute = q.getval();
        bp.closeYear = q.getval();
        bp.closeMonth = q.getval();
        bp.closeDay = q.getval();
        bp.closeHour = q.getval();
        bp.closeMinute = q.getval();
        bp.memo = q.getstr();
        packs[name] = bp;
    }
    q.free_result();

    return 0;
}

int merge_base_packs(std::map<std::string, base_pack>& packs1, std::map<std::string, base_pack>& packs2, std::map<std::string, base_pack>& packs)
{
    int id = 0;
    for (std::map<std::string, base_pack>::iterator it = packs1.begin(); it != packs1.end(); ++it)
    {
        base_pack bp;
        if (packs2.find(it->first) != packs2.end())
        {
            bp = packs2[it->first];
        }
        else
        {
            bp = it->second;
        }
        ++id;
        bp.pid = id;
        packs[it->first] = bp;
    }
    return 0;
}

int merge_packs(int max_cid, const std::string& from_db, const std::string& to_db, Query& q)
{
    uint64_t time_start = printTimeElapsed(false);
    cout<<"start merge libao ..."<<endl;

    cout<<"\t\t\tprocess libao id ...";
    //最大礼包id
    int max_pid = 0;
    q.get_result("select max(id) from " + to_db + ".custom_packs where 1");
    if (q.fetch_row())
    {
        max_pid = q.getval();
    }
    q.free_result();
    q.get_result("select max(id) from " + from_db + ".custom_packs where 1");
    if (q.fetch_row())
    {
        int id = q.getval();
        if (id > max_pid)
        {
            max_pid = id;
        }
    }
    q.free_result();

    int max_base_pid = 0;
    q.get_result("select max(id) from " + to_db + ".custom_base_packs where 1");
    if (q.fetch_row())
    {
        max_base_pid = q.getval();
    }
    q.free_result();
    q.get_result("select max(id) from " + from_db + ".custom_base_packs where 1");
    if (q.fetch_row())
    {
        int id = q.getval();
        if (id > max_base_pid)
        {
            max_base_pid = id;
        }
    }
    q.free_result();

    //礼包id处理
    if (!q.execute("update " + from_db + ".custom_packs set id=id+" + LEX_CAST_STR(max_pid)
                    + ",pid=pid+" + LEX_CAST_STR(max_base_pid) + " where 1"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".custom_base_packs set id=id+" + LEX_CAST_STR(max_base_pid) + " where 1"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    //礼包领取记录表处理
    if (!q.execute("update " + from_db + ".char_opened_packs set pid=pid+" + LEX_CAST_STR(max_pid)
            + ",cid=cid+" + LEX_CAST_STR(max_cid)
            + " where 1"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }

    std::map<int,int> pid_map;
    q.get_result("select o1.id,o2.id from " + from_db + ".custom_packs as o1 left join " + to_db + ".custom_packs as o2 on o1.name=o2.name where 1");
    while (q.fetch_row())
    {
        int pid1 = q.getval();
        int pid2 = q.getval();

        if (pid1 > 0 && pid2 > 0)
        {
            pid_map[pid1] = pid2;
        }
    }
    q.free_result();

    for (std::map<int,int>::iterator it = pid_map.begin(); it != pid_map.end(); ++it)
    {
        //礼包领取记录表处理
        if (!q.execute("update " + from_db + ".char_opened_packs set pid=" + LEX_CAST_STR(it->second)
                + " where pid=" + LEX_CAST_STR(it->first)))
        {
            CHECK_DB_ERR(q);
            return -1;
        }
    }

    //礼包物品表处理
    if (!q.execute("update " + from_db + ".custom_base_packs_items set pid=pid+" + LEX_CAST_STR(max_base_pid)
            + " where 1"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }

    printTimeElapsed();

    cout<<"\t\t\tmerge table custom_base_packs_items...";
    //合并
    if (!q.execute("INSERT INTO " + to_db + ".`custom_base_packs_items`  (aid,pid,type,id,count) SELECT NULL,pid,type,id,count FROM  " + from_db + ".`custom_base_packs_items`"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"\t\t\tmerge table custom_base_packs...";
    if (!q.execute("INSERT INTO " + to_db + ".`custom_base_packs` SELECT * FROM  " + from_db + ".`custom_base_packs` where name not in (select name from " + to_db + ".custom_base_packs where 1)"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"\t\t\tmerge table custom_packs...";
    if (!q.execute("INSERT INTO " + to_db + ".`custom_packs` SELECT * FROM  " + from_db + ".`custom_packs` where name not in (select name from " + to_db + ".custom_packs where 1)"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    //合并
    cout<<"\t\t\tmerge table char_opened_packs...";
    if (!q.execute("INSERT INTO " + to_db + ".`char_opened_packs` SELECT * FROM  " + from_db + ".`char_opened_packs`"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();
    //激活码批次
    cout<<"\t\t\tupdate table char_opened_packs...";
    if (!q.execute("update " + to_db + ".`char_opened_packs` as cp left join " + to_db + ".`admin_code` as ac on ac.uniques=cp.code set cp.seqNo=ac.batch where cp.seqNo!=''"))
    {
        CHECK_DB_ERR(q);
        //return -1;
    }
    printTimeElapsed();

    cout<<"merge libao complete!\t";
    printTimeElapsed(true, time_start);
    return 0;
}

int getMaxId(const std::string& from_db, const std::string& to_db, const std::string& table, const std::string& field, Query& q)
{
    int max_id = 0;
    q.get_result("select max(`" + field + "`) from `" + to_db + "`.`" + table + "` where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        max_id = q.getval();
    }
    q.free_result();
    q.get_result("select max(`" + field + "`) from `" + from_db + "`.`" + table + "` where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        int cid = q.getval();
        if (cid > max_id)
        {
            max_id = cid;
        }
    }
    q.free_result();
    return max_id;
}

int addTableId(const std::string& db, const std::string& table, const std::string& id, int add, Query& q)
{
    if ((table == "char_baoshi" && id == "gid") || (table == "char_equipment" && id == "gid"))
    {
        ;
    }
    else if (!q.execute("delete from `" + db + "`.`" + table + "` where `" + id + "`=0"))
    {
        CHECK_DB_ERR(q);
    }
    if (!q.execute("update `" + db + "`.`" + table + "` set `" + id + "`=`" + id + "`+" + LEX_CAST_STR(add) + " where 1"))
    {
        CHECK_DB_ERR(q);
        if (q.GetErrno() == db_error_table_not_exit)
        {
            return 0;
        }
        return q.GetErrno();
    }
    return 0;
}

/*
    id : "" 表示不处理这个字段
    cid : 0 表示 不处理cid
    force_merge: true表示强制合并  false表示重复的不合并
*/
int mergeTable(const std::string& from_db, const std::string& to_db, const std::string& table, const std::string id, Query& q, int cid = 0, const std::string& unique_field = "")
{
    cout<<"starti merge table "<<table<<"..."<<endl;
    printTimeElapsed(false);
    int ret = 0;
    if (id != "")
    {
        int max_id = getMaxId(from_db, to_db, table, id, q);
        ret = addTableId(from_db, table,id, max_id, q);
    }
    if (ret == 0)
    {
        if (cid)
        {
            ret = addTableId(from_db, table,"cid", cid, q);
            if (ret == 0)
            {

            }
            else
            {
                cout<<"!!!!!!!!!!!!!! motify "<<table<<".cid fail,error:"<<q.GetError()<<endl;
                return ret;
            }
        }
        //merge
        if (unique_field == "")
        {
            if (!q.execute("INSERT INTO `" + to_db + "`.`" + table + "` SELECT * FROM  `" + from_db + "`.`" + table + "`"))
            {
                CHECK_DB_ERR(q);
                if (q.GetErrno() == db_error_table_not_exit)
                {
                }
                else
                {
                    cout<<"!!!!!!!!!!!!!! merge table fail :"<<table<<",error:"<<q.GetError()<<endl;
                    return -1;
                }
            }
        }
        else
        {
            if (!q.execute("INSERT INTO `" + to_db + "`.`" + table
                    + "` SELECT * FROM `" + from_db + "`.`" + table + "` where `" + unique_field
                    + "` not in (select `" + unique_field + "` from `" + to_db + "`.`" + table + "`)"))
            {
                CHECK_DB_ERR(q);
                if (q.GetErrno() == db_error_table_not_exit)
                {
                }
                else
                {
                    cout<<"!!!!!!!!!!!!!! merge table fail :"<<table<<",error:"<<q.GetError()<<endl;
                    return -1;
                }
            }
        }
        cout<<"\t\t\t"<<table<<"\t\tmerged ";
        printTimeElapsed();
        return 0;
    }
    else
    {
        cout<<"!!!!!!!!!!!!!! motify "<<table<<"." + id + " fail,error:"<<q.GetError()<<endl;
        return ret;
    }
}

int mergeTable2(const std::string& from_db, const std::string& to_db, const std::string& table, const std::string& sql1, const std::string& sql2, Query& q)
{
    if (!q.execute("INSERT INTO `" + to_db + "`.`" + table + "` " + sql1 + " (SELECT " + sql2 + " FROM  `" + from_db + "`.`" + table + "`)"))
    {
        CHECK_DB_ERR(q);
        if (q.GetErrno() == db_error_table_not_exit)
        {
        }
        else
        {
            cout<<"!!!!!!!!!!!!!! merge table fail :"<<table<<",error:"<<q.GetError()<<endl;
            return -1;
        }
    }
    return 0;
}

//检查表的异常
int doCheck(const std::string& from_db, const std::string& to_db, Query& q)
{
    //检查礼包名字的唯一性
    cout<<"\t\t\tcheck libao name unique ...";
    q.get_result("SELECT COUNT( * ),name AS c FROM  " + from_db + ".`custom_packs` WHERE 1 GROUP BY name order by c desc");
    if (q.fetch_row() && q.getval() > 1)
    {
        cout<<"db:"<<from_db<<",pack name not unique. name"<<q.getstr()<<endl;
        return -1;
    }

    q.get_result("SELECT COUNT( * ),name AS c FROM  " + to_db + ".`custom_packs` WHERE 1 GROUP BY name order by c desc");
    if (q.fetch_row() && q.getval() > 1)
    {
        cout<<"db:"<<to_db<<",pack name not unique. name"<<q.getstr()<<endl;
        return -1;
    }
    return 0;
}

int merge(const std::string& from_db, const std::string& to_db, const std::string& suffix, bool merge_only)
{
    Database db("localhost", "c_user", "23rf234", "", 0, 3306);
    Query q(db);

    uint64_t time_start = printTimeElapsed(false);

    if (doCheck(from_db, to_db, q) != 0)
    {
        return -1;
    }

if (!merge_only)
{
    q.execute("delete from " + from_db + ".char_default_zhen where cid not in (select id from " + from_db + ".charactors)");
    CHECK_DB_ERR(q);
    q.execute("delete from " + to_db + ".char_default_zhen where cid not in (select id from " + to_db + ".charactors)");
    CHECK_DB_ERR(q);

    q.execute("ALTER TABLE  " + from_db + ".`char_stronghold_states` DROP    `id`");
    CHECK_DB_ERR(q);
    q.execute("ALTER TABLE  " + to_db + ".`char_stronghold_states` DROP    `id`");
    CHECK_DB_ERR(q);

    q.execute("ALTER TABLE  " + from_db + ".`admin_char` DROP    `id`");
    CHECK_DB_ERR(q);
    q.execute("ALTER TABLE  " + to_db + ".`admin_char` DROP    `id`");
    CHECK_DB_ERR(q);

    cout<<"check charactor name unique...";
    //角色名字唯一性
    if (!q.execute("update " + from_db + ".charactors left join " + to_db + ".charactors on " + from_db + ".charactors.name=" + to_db + ".charactors.name set " + from_db + ".charactors.name=CONCAT(" + from_db + ".charactors.name,'" + suffix + "') WHERE " + to_db + ".charactors.name!=''"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    //再执行一次
    if (!q.execute("update " + from_db + ".charactors left join " + to_db + ".charactors on " + from_db + ".charactors.name=" + to_db + ".charactors.name set " + from_db + ".charactors.name=CONCAT(" + from_db + ".charactors.name,'" + suffix + "') WHERE " + to_db + ".charactors.name!=''"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"check corps name unique...";
    //军团名字唯一性
    if (!q.execute("update " + from_db + ".char_corps left join " + to_db + ".char_corps on " + from_db + ".char_corps.name=" + to_db + ".char_corps.name set " + from_db + ".char_corps.name=CONCAT(" + from_db + ".char_corps.name,'" + suffix + "') WHERE " + to_db + ".char_corps.name!=''"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    //再执行一次
    if (!q.execute("update " + from_db + ".char_corps left join " + to_db + ".char_corps on " + from_db + ".char_corps.name=" + to_db + ".char_corps.name set " + from_db + ".char_corps.name=CONCAT(" + from_db + ".char_corps.name,'" + suffix + "') WHERE " + to_db + ".char_corps.name!=''"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();
}
    //目标服的最大角色id
    int max_cid = getMaxId(from_db, to_db, "charactors", "id", q);

if (!merge_only)
{
    if (merge_packs(max_cid, from_db, to_db, q) < 0)
    {
        cout<<"merge libao fail."<<endl;
        return -1;
    }

    cout<<"process charactor id ...";
    //修改 from_db 相关表的角色id
    if (addTableId(from_db, "charactors", "id", max_cid, q))
    {
        return -1;
    }
    printTimeElapsed();

    std::string char_tables[] =
    {
        "char_all_rewards",//2.11
        "char_baoshi",
        "char_baoshi_events",//2.11
        "char_boss_damage_rankings",
        "char_buffs",//2.12
        "char_camp_race_rankings",
        "char_continue_login_present",
        "char_copy_attack",
        "char_corps_applications",
        "char_corps_event",
        "char_corps_history",
        "char_corps_members",
        "char_daily_recharge",//2.12
        "char_daily_task",
        "char_daily_temp",
        "char_data",
        "char_data_extra",
        "char_data_temp",
        "char_default_zhen",
        "char_elite_map_tempo",
        "char_elite_tempo",
        //"char_equipts",//delete
        "char_equipment",
        "char_explore_can",
        "char_explore_has",
        "char_farm_field",
        "char_feedback",//2.11
        "char_friends",
        "char_generals",
        "char_general_events",//2.11
        // char_generals_genius_lock -> nouse
        "char_gm_list",
        "char_gm_question",
        "char_goldCost_noConfirm",
        "char_guard_goods",
        "char_guide_complete",
        "char_horses",
        "char_horses_action",
        "char_lottery_records",
        "char_map_intro_get",
        "char_newbie_event",
        "char_new_states",
        "char_new_weapons",
        "char_offical_skills",
        //"char_opened_packs",
        "char_presents",
        "char_race",
        //"char_race_title",
        "char_recharge",
        "char_recharge_event",
        "char_recharge_event_records",
        "char_recharge_event_total",
        "char_resource",
        "char_servant",
        "char_servant_enemy",
        "char_servant_event",
        "char_servant_loser",
        "char_shop_goods",
        "char_signs",//2.11
        "char_skills",
        "char_skill_research",
        "char_skill_teachers",
        "char_smelt",
        "char_smelt_task",
        "char_stand_in",
        "char_stronghold",
        "char_stronghold_states",
        //"char_stronghold_times",
        "char_sweep_result",
        "char_sweep_task",
        "char_tasks",
        "char_tmp_vip",
        "char_total_recharge",
        "char_training",
        "char_train_books",
        "char_train_place",
        "char_treasures",
        "char_trunk_tasks",
        "char_vip_present",
        "char_year_temp",
        "char_zhens",
        //2.0
        "char_trades",
        //"char_new_rankings_last",
        //"char_new_rankings_now",
        "char_libao",
        "char_enemy_infos",
        "char_congratulations",
        "char_recved_congratulations",
        //"char_guard_rankget",
        "char_guard_rankscore",
        "char_farm_water",
        "char_enemys",
        "char_bank_cases",
        //2.13
        "char_jxl_buffs",
        //2.1.3.3
        "char_corps_fighting",
        "char_chengzhang_event",

        //2.1.4
        "char_zst",
        "char_zst_generals",
        "char_zst_maps",

        ""
    };
    for (int i = 0; char_tables[i] != ""; ++i)
    {
        cout<<"\t\tprocess table "<<char_tables[i]<<"..."<<flush;
        int ret = addTableId(from_db, char_tables[i], "cid", max_cid, q);
        if (ret > 0 && ret != db_error_table_not_exit)
        {
            return -1;
        }
        printTimeElapsed();
    }
    cout<<"process special tables..."<<endl;

    cout<<"\t\tprocess char_corps..."<<flush;
    /*特殊的字段名不是cid    char_corps: creater*/
    if (addTableId(from_db, "char_corps", "creater", max_cid, q))
    {
        return -1;
    }
    printTimeElapsed();
    //char_mails: cid,from
    cout<<"\t\tprocess char_mails.cid..."<<flush;
    if (!q.execute("update " + from_db + ".char_mails set cid=cid+" + LEX_CAST_STR(max_cid) + " where cid>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"\t\tprocess char_mails.from..."<<flush;
    if (!q.execute("update " + from_db + ".char_mails set `from`=`from`+" + LEX_CAST_STR(max_cid) + " where `from`>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"\t\tprocess char_servant.master_id..."<<flush;
    //char_servant:master_id
    if (!q.execute("update " + from_db + ".char_servant set master_id=master_id+" + LEX_CAST_STR(max_cid) + " where master_id>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"\t\tprocess char_servant_enermy..."<<flush;
    //char_servant_enemy:enemyid
    if (!q.execute("update " + from_db + ".char_servant_enemy set enemyid=enemyid+" + LEX_CAST_STR(max_cid) + " where enemyid>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"\t\tprocess char_servant_loser..."<<flush;
    //char_servant_loser:loserid
    if (!q.execute("update " + from_db + ".char_servant_loser set loserid=loserid+" + LEX_CAST_STR(max_cid) + " where loserid>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();
    cout<<"\t\tprocess char_camp_race_rankings..."<<endl;

    cout<<"\t\tprocess char_friends..."<<flush;
    //char_friends:friend_id
    if (!q.execute("update " + from_db + ".char_friends set friend_id=friend_id+" + LEX_CAST_STR(max_cid) + " where friend_id>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();
    cout<<"\t\tprocess char_camp_race_rankings..."<<endl;
    int max_camp_race_rankings_id = getMaxId(from_db, to_db, "char_camp_race_rankings", "id", q);
    if (addTableId(from_db, "char_camp_race_rankings", "id", max_camp_race_rankings_id, q))
    {
        return -1;
    }
    printTimeElapsed();

    cout<<"process mail id ..."<<flush;
    /*********************目标服最大邮件id ***************************/
    int max_mail_id = getMaxId(from_db, to_db, "char_mails", "id", q);

    //char_mails: id
    if (addTableId(from_db, "char_mails", "id", max_mail_id, q))
    {
        return -1;
    }
    printTimeElapsed();


    cout<<"process corps id ..."<<flush;
    /**********************目标服最大工会id *************************/
    int max_corps_id = getMaxId(from_db, to_db, "char_corps", "id", q);

    //char_corps: id
    if (addTableId(from_db, "char_corps", "id", max_corps_id, q))
    {
        return -1;
    }

    //char_corps_applications.corps
    if (addTableId(from_db, "char_corps_applications", "corps", max_corps_id, q))
    {
        return -1;
    }

    //char_corps_event.corps
    if (addTableId(from_db, "char_corps_event", "corps", max_corps_id, q))
    {
        return -1;
    }

    //char_corps_history.corps
    if (addTableId(from_db, "char_corps_history", "corps", max_corps_id, q))
    {
        return -1;
    }
    //char_corps_members.corps
    if (addTableId(from_db, "char_corps_members", "corps", max_corps_id, q))
    {
        return -1;
    }
    if (addTableId(from_db, "char_corps_fighting", "corps", max_corps_id, q))
    {
        return -1;
    }
    if (addTableId(from_db, "char_corps_boss", "corps", max_corps_id, q))
    {
        return -1;
    }
    printTimeElapsed();

    cout<<"process equiptment id ..."<<flush;
    //目标服最大装备id
    int max_eid = getMaxId(from_db, to_db, "char_equipment", "id", q);

    if (addTableId(from_db, "char_equipment", "id", max_eid, q))
    {
        return -1;
    }
    printTimeElapsed();

    cout<<"process baoshi id ..."<<flush;

    //目标服最大宝石id
    int max_baoshi_id = getMaxId(from_db, to_db, "char_baoshi", "id", q);

    if (addTableId(from_db, "char_baoshi", "id", max_baoshi_id, q))
    {
        return -1;
    }
    printTimeElapsed();

    cout<<"process general id ..."<<flush;

    //目标服最大武将id
    int max_gid = getMaxId(from_db, to_db, "char_generals", "id", q);

    //char_generals.id
    if (addTableId(from_db, "char_generals", "id", max_gid, q))
    {
        return -1;
    }
    if (addTableId(from_db, "char_baoshi", "gid", max_gid, q))
    {
        return -1;
    }
    if (addTableId(from_db, "char_equipment", "gid", max_gid, q))
    {
        return -1;
    }
    if (addTableId(from_db, "char_zst_generals", "guid", max_gid, q))
    {
        return -1;
    }

    //char_zhens.pos1-pos9
    if (!q.execute("update " + from_db + ".char_zhens set pos1=pos1+" + LEX_CAST_STR(max_gid) + " where pos1>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos2=pos2+" + LEX_CAST_STR(max_gid) + " where pos2>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos3=pos3+" + LEX_CAST_STR(max_gid) + " where pos3>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos4=pos4+" + LEX_CAST_STR(max_gid) + " where pos4>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos5=pos5+" + LEX_CAST_STR(max_gid) + " where pos5>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos6=pos6+" + LEX_CAST_STR(max_gid) + " where pos6>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos7=pos7+" + LEX_CAST_STR(max_gid) + " where pos7>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos8=pos8+" + LEX_CAST_STR(max_gid) + " where pos8>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + from_db + ".char_zhens set pos9=pos9+" + LEX_CAST_STR(max_gid) + " where pos9>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }

    if (!q.execute("update " + from_db + ".char_train_place set gid=gid+" + LEX_CAST_STR(max_gid) + " where gid>0"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    cout<<"process recharge id ..."<<flush;
    // char_recharge 最大自增id
    int max_recharge_id = getMaxId(from_db, to_db, "char_recharge", "id", q);

    //修改 from_db char_recharge 自增id
    if (addTableId(from_db, "char_recharge", "id", max_recharge_id, q))
    {
        return -1;
    }
    printTimeElapsed();

    cout<<"process pay_list id ..."<<flush;
    //pay_list 自增处理
    int max_pay_id = getMaxId(from_db, to_db, "pay_list", "pay_id", q);

    //修改 from_db char_recharge 自增id
    if (addTableId(from_db, "pay_list", "pay_id", max_pay_id, q))
    {
        return -1;
    }
    printTimeElapsed();


    /*2.0-------
        "char_trades",
        "char_libao",
        "char_friend_infos",
        "char_enemy_infos",
        "char_congratulations",
        "char_recved_congratulations",
        "char_guard_rankget",
        "char_guard_rankscore",
        "char_farm_water",
        "char_enemys",
        "char_bank_cases",*/

    cout<<"process char_libao id ..."<<flush;
    //char_libao 自增处理
    int max_libao_id = getMaxId(from_db, to_db, "char_libao", "id", q);

    //修改 from_db char_libao 自增id
    if (addTableId(from_db, "char_libao", "id", max_libao_id, q))
    {
        return -1;
    }
    printTimeElapsed();

    if (addTableId(from_db, "char_enemys", "eid", max_cid, q))
    {
        return -1;
    }
    if (addTableId(from_db, "char_enemy_infos", "eid", max_cid, q))
    {
        return -1;
    }
    {
        int max_einfo_id = getMaxId(from_db, to_db, "char_enemy_infos", "id", q);

        //修改 from_db char_enemy_infos 自增id
        if (addTableId(from_db, "char_enemy_infos", "id", max_einfo_id, q))
        {
            return -1;
        }
    }
    if (addTableId(from_db, "char_farm_water", "friend_id", max_cid, q))
    {
        return -1;
    }
    //char_congratulations
    if (addTableId(from_db, "char_congratulations", "fid", max_cid, q))
    {
        return -1;
    }
    {
        int max_finfo_id = getMaxId(from_db, to_db, "char_congratulations", "id", q);

        //修改 from_db char_congratulations 自增id
        if (addTableId(from_db, "char_congratulations", "id", max_finfo_id, q))
        {
            return -1;
        }
    }
    //char_recved_congratulations
    if (addTableId(from_db, "char_recved_congratulations", "fid", max_cid, q))
    {
        return -1;
    }
    {
        int max_finfo_id = getMaxId(from_db, to_db, "char_recved_congratulations", "id", q);

        //修改 from_db char_recved_congratulations 自增id
        if (addTableId(from_db, "char_recved_congratulations", "id", max_finfo_id, q))
        {
            return -1;
        }
    }

    cout<<"process race rankings ..."<<flush;
    //竞技场排名需要处理下
    if (!q.execute("update " + from_db + ".char_race set rank=2*rank where 1"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    if (!q.execute("update " + to_db + ".char_race set rank=2*rank-1 where 1"))
    {
        CHECK_DB_ERR(q);
        return -1;
    }
    printTimeElapsed();

    int max_gmq_id = getMaxId(from_db, to_db, "char_gm_question", "id", q);
    if (addTableId(from_db, "char_gm_question", "id", max_gmq_id, q))
    {
        return -1;
    }
    printTimeElapsed();

    int max_treasure_id = getMaxId(from_db, to_db, "char_treasures", "id", q);
    if (addTableId(from_db, "char_treasures", "id", max_treasure_id, q))
    {
        return -1;
    }
    printTimeElapsed();
}

    /*合并数据

    INSERT INTO  `test2`.`charactors`
    SELECT *
    FROM  `test1`.`charactors` ;

    */
    cout<<"staring merge tables ..."<<endl;
    std::string merge_tables[] =
    {
        "charactors",
        "char_all_rewards",//2.11
        "char_baoshi",
        "char_baoshi_events",//2.11
        "char_boss_damage_rankings",
        "char_buffs",//2.12
        "char_camp_race_rankings",
        "char_continue_login_present",
        "char_copy_attack",
        "char_corps",
        "char_corps_applications",
        "char_corps_event",
        "char_corps_history",
        "char_corps_members",
        "char_daily_recharge",//2.12
        "char_daily_task",
        "char_daily_temp",
        "char_data",
        "char_data_extra",
        "char_data_temp",
        "char_default_zhen",
        "char_elite_map_tempo",
        "char_elite_tempo",
        "char_equipment",
        "char_explore_can",
        "char_explore_has",
        "char_farm_field",
        "char_feedback",//2.11
        "char_friends",
        "char_generals",
        "char_general_events",//2.11
        "char_gm_list",
        "char_gm_question",
        "char_goldCost_noConfirm",
        "char_guard_goods",
        "char_guide_complete",
        "char_horses",
        "char_horses_action",
        "char_lottery_records",
        "char_mails",
        "char_map_intro_get",
        "char_newbie_event",
        "char_new_states",
        "char_new_weapons",
        "char_offical_skills",
        //"char_opened_packs",
        "char_presents",
        "char_race",
        "char_recharge",
        "char_recharge_event",
        "char_recharge_event_records",
        "char_recharge_event_total",
        "char_resource",
        "char_servant",
        "char_servant_enemy",
        "char_servant_event",
        "char_servant_loser",
        "char_shop_goods",
        "char_signs",//2.11
        "char_skills",
        "char_skill_research",
        "char_skill_teachers",
        "char_smelt",
        "char_smelt_task",
        "char_stand_in",
        "char_stronghold",
        "char_stronghold_states",
        //"char_stronghold_times",
        "char_sweep_result",
        "char_sweep_task",
        "char_tasks",
        "char_tmp_vip",
        "char_total_recharge",
        "char_training",
        "char_train_books",
        "char_train_place",
        "char_treasures",
        "char_trunk_tasks",
        "char_vip_present",
        "char_year_temp",
        "char_zhens",
        "pay_list",
        //2.0
        "char_trades",
        //"char_new_rankings_last",
        //"char_new_rankings_now",
        "char_libao",
        "char_enemy_infos",
        "char_congratulations",
        "char_recved_congratulations",
        //"char_guard_rankget",
        "char_guard_rankscore",
        "char_farm_water",
        "char_enemys",
        "char_bank_cases",
        //2.13
        "char_jxl_buffs",
        //2.1.3.3
        "char_corps_fighting",
        "char_chengzhang_event",
        "char_corps_boss",

        //2.1.4
        "char_zst",
        "char_zst_generals",
        "char_zst_maps",
        ""
    };
    for (int i = 0; merge_tables[i] != ""; ++i)
    {
        if (!q.execute("INSERT INTO  " + to_db + "." + merge_tables[i] + " SELECT * FROM  " + from_db + "." + merge_tables[i]))
        {
            CHECK_DB_ERR(q);
            if (q.GetErrno() == db_error_table_not_exit)
            {
            }
            else
            {
                return -1;
            }
        }
        cout<<"\t\t\t"<<merge_tables[i]<<"\t\tmerged ";
        printTimeElapsed();
    }

/*
    accounts_adv 取并集
    admin_char
    admin_costgold
    admin_costsill
    admin_count_flat
    admin_count_ling
    admin_count_market
    admin_count_smost
    admin_pack
    admin_resources
    admin_scoll 取并集
    admin_sougold
    admin_sousill
    admin_speak
    admin_login
    admin_first2
    admin_first2stat
    admin_firststat
    */
    cout<<"start merge admin_xx tables ..."<<endl;
    //cid要加的
    std::string admin_tables[][3] =
    {
        {"admin_char", "", ""},
        {"admin_pack","id", ""},
        {"admin_resources","id", ""},
        {"admin_speak","id", ""},
        //{"admin_firststat","id", ""},
        {"", "", ""}
    };
    for (int i = 0; admin_tables[i][0] != ""; ++i)
    {
        mergeTable(from_db, to_db, admin_tables[i][0], admin_tables[i][1], q, max_cid, admin_tables[i][2]);
    }
    //特殊的，合并的时候，自增id特殊处理
    addTableId(from_db, "admin_costgold", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_costgold", "(id,cid,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,hnums,stime,ip,type,union_id,server_id", q);

    addTableId(from_db, "admin_costsill", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_costsill", "(id,cid,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,hnums,stime,ip,type,union_id,server_id", q);

    addTableId(from_db, "admin_count_flat", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_count_flat", "(id,cid,name,ftype,type,eid,hnums,xnums,stime,etime,ip,isdone)", "NULL,cid,name,ftype,type,eid,hnums,xnums,stime,etime,ip,isdone", q);

    addTableId(from_db, "admin_count_ling", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_count_ling", "(id,cid,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,hnums,stime,ip,type,union_id,server_id", q);

    addTableId(from_db, "admin_count_market", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_count_market", "(id,cid,mid,aunit,nums,dtime,types,ip,isdone)", "NULL,cid,mid,aunit,nums,dtime,types,ip,isdone", q);

    addTableId(from_db, "admin_count_smost", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_count_smost", "(id,cid,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,hnums,stime,ip,type,union_id,server_id", q);

    addTableId(from_db, "admin_sougold", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_sougold", "(id,cid,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,hnums,stime,ip,type,union_id,server_id", q);

    addTableId(from_db, "admin_sousill", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_sousill", "(id,cid,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,hnums,stime,ip,type,union_id,server_id", q);

    addTableId(from_db, "admin_count_baoshi", "cid", max_cid, q);
    mergeTable2(from_db, to_db, "admin_count_baoshi", "(id,cid,stype,btype,level,hnums,stime,ip,type,union_id,server_id)", "NULL,cid,stype,btype,level,hnums,stime,ip,type,union_id,server_id", q);

    //cid不需要处理的
    mergeTable(from_db, to_db, "admin_login","id", q, 0);
    mergeTable(from_db, to_db, "accounts_all","id", q, 0);
    //mergeTable(from_db, to_db, "accounts_plat","id", q, 0);
    if (false == q.execute("INSERT INTO " + to_db + ".accounts_plat (union_id,qid,qname,adv_id,adv_sid,unionid,time_reg) \
                                SELECT b.union_id,b.qid,b.qname,b.adv_id,b.adv_sid,b.unionid,b.time_reg \
                                FROM " + from_db + ".accounts_plat AS b \
                                LEFT JOIN " + to_db + ".accounts_plat AS c ON b.union_id=c.union_id AND b.qid=c.qid \
                                WHERE c.id IS NULL"))
    {
        CHECK_DB_ERR(q);
    }
    mergeTable(from_db, to_db, "admin_acode","id", q, 0);
    mergeTable(from_db, to_db, "admin_course_data","id", q, 0);
    mergeTable(from_db, to_db, "admin_log","log_id", q, 0);
    //mergeTable(from_db, to_db, "admin_first2","id", q, 0);
    //mergeTable(from_db, to_db, "admin_first2stat","id", q, 0);
    //不重复的合并
    mergeTable(from_db, to_db, "admin_scoll","id", q, 0, "account");
    mergeTable(from_db, to_db, "accounts","id", q, 0);

    if (false == q.execute("TRUNCATE TABLE `" + to_db + "`.`throne_rankings_last`"))
    {
        CHECK_DB_ERR(q);
    }

    cout<<"merge complete!\t";
    printTimeElapsed(true, time_start);
    return 0;
}


/* shhx_merge db_from db_to prefix */
int main(int argc, char* argv[])
{
    if (argc <= 3)
    {
        cout<<"shhx_merge db_from db_to prefix."<<endl;
        return 1;
    }
    if (argc == 4)
    {
        return merge(argv[1], argv[2], argv[3], false);
    }
    else
    {
        return merge(argv[1], argv[2], argv[3], true);
    }
}

