#pragma once

#include "json_spirit.h"
#include <string>
#include <map>
#include <list>
#include "utils_all.h"

class combatRecordMgr
{
public:
    combatRecordMgr();
    std::string getCombatRecord(int id);
    void addCombatRecord(int id, const std::string& record);
private:
    std::list<int> m_id_list;
    std::map<int, std::string> m_battle_records;

    rwlock m_lock;
};

