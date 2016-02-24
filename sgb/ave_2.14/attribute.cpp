
#include "attribute.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
Database& GetDb();

attributeMgr* attributeMgr::m_handle = NULL;

attributeMgr* attributeMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new attributeMgr();
        m_handle->reload();
    }
    return m_handle;
}

void attributeMgr::reload()
{
    Query q(GetDb());
    q.get_result("select type,name,isChance from base_attribute where 1");
    while (q.fetch_row())
    {
        boost::shared_ptr<baseAttribute> bsa(new baseAttribute);
        bsa->type = q.getval();
        bsa->name = q.getstr();
        bsa->isChance = q.getval();
        m_attributes[bsa->type] = bsa;
    }
    q.free_result();
}

boost::shared_ptr<baseAttribute> attributeMgr::getAttribute(int type)
{
    std::map<int, boost::shared_ptr<baseAttribute> >::iterator it = m_attributes.find(type);
    if (it != m_attributes.end())
    {
        return it->second;
    }
    boost::shared_ptr<baseAttribute> bsa;
    return bsa;
}

