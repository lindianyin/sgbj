#pragma once

#include <string>
#include <map>
#include <list>
#include "utils_all.h"
#include "json_spirit.h"
#include "net.h"

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

//Õ½¶·»Ø·Å
int ProcessGetCombatRecord(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj);

