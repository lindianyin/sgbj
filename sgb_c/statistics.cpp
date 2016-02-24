
/********* 数据统计 ************/

#include "statistics.h"
#include "utils_all.h"

#define ENABLE_STATISTICS

extern void InsertSaveDb(const std::string& sql);

//金币获得统计
void statistics_of_gold_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == gold_get_init)
        return;
    InsertSaveDb("insert into admin_get_gold (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//金币消耗统计
void statistics_of_gold_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == gold_cost_init)
        return;
    InsertSaveDb("insert into admin_cost_gold (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//银币获得统计
void statistics_of_silver_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == silver_get_init)
        return;
    InsertSaveDb("insert into admin_get_silver (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//银币消耗统计
void statistics_of_silver_cost(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == silver_cost_init)
        return;
    InsertSaveDb("insert into admin_cost_silver (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//声望获得统计
void statistics_of_prestige_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == 0)
        return;
    InsertSaveDb("insert into admin_get_prestige (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}
//历练获得统计
void statistics_of_char_exp_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == 0)
        return;
    InsertSaveDb("insert into admin_get_char_exp (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}
//经验统计
void statistics_of_hero_exp_get(int cid, const std::string& strIP, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == 0)
        return;
    InsertSaveDb("insert into admin_get_hero_exp (id,cid,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//宝物获得统计
void statistics_of_gem_get(int cid, const std::string& strIP, int gem_id, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == 0)
        return;
    InsertSaveDb("insert into admin_get_gem (id,cid,gem_id,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(gem_id) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//宝物消耗统计
void statistics_of_gem_cost(int cid, const std::string& strIP, int gem_id, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == 0)
        return;
    InsertSaveDb("insert into admin_cost_gem (id,cid,gem_id,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(gem_id) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

//英雄获得统计
void statistics_of_hero_get(int cid, const std::string& strIP, int hero_id, int counts, int type, int union_id, const std::string& server_id)
{
#ifdef ENABLE_STATISTICS
    if (type == 0)
        return;
    InsertSaveDb("insert into admin_get_hero (id,cid,hero_id,hnums,stime,ip,type,union_id,server_id) values (NULL," + LEX_CAST_STR(cid) + ","
        + LEX_CAST_STR(hero_id) + ","
        + LEX_CAST_STR(counts) + ",now(),'" + strIP + "'," + LEX_CAST_STR(type) + ","
        + LEX_CAST_STR(union_id) + ",'" + server_id + "')");
#endif
}

