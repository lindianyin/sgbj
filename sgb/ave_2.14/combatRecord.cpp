
#include "combatRecord.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "utils_all.h"
#include "spls_errcode.h"

Database& GetDb();

combatRecordMgr::combatRecordMgr()
{
    Query q(GetDb());
    q.get_result("select id,record from battle_records where archive='1' limit 5000");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        m_id_list.push_back(id);
        m_battle_records[id] = q.getstr();
    }
    q.free_result();

    rwlock_init(&m_lock);
}

std::string combatRecordMgr::getCombatRecord(int id)
{
    rwlock_rlock(&m_lock);
    std::map<int, std::string>::iterator it = m_battle_records.find(id);
    if (it != m_battle_records.end())
    {
        std::string ret = it->second;
        rwlock_runlock(&m_lock);
        return ret;
    }
    rwlock_runlock(&m_lock);
    Query q(GetDb());
    q.get_result("select record from battle_records where id=" + LEX_CAST_STR(id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        std::string result = q.getstr();
        q.free_result();
        return result;
    }
    else
    {
        q.free_result();
        ERR();
        cout<<"battle record not exist:"<<id<<endl;
        return "";
    }
}

void combatRecordMgr::addCombatRecord(int id, const std::string& record)
{
    rwlock_wlock(&m_lock);
    m_id_list.push_back(id);
    m_battle_records[id] = record;
    if (m_id_list.size() >= 5000)
    {
        int id = m_id_list.front();
        m_battle_records.erase(id);
        m_id_list.pop_front();
    }
    rwlock_wunlock(&m_lock);
}

