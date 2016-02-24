#include "statistics.h"

#include "packs.h"
#include "spls_errcode.h"
#include "net.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "utils_all.h"

#define INFO(x) cout<<x

class Combat;

using namespace net;

extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool, int statistics_type);
Database& GetDb();

extern void InsertSaveDb(const std::string& sql);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);

extern std::string strPack_needMoreLevel;
extern std::string strPack_needMorePrestige;
extern std::string strPack_needMoreVIP;

packsMgr* packsMgr::m_handle = NULL;
packsMgr* packsMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new packsMgr();
        m_handle->reload();
    }
    return m_handle;
}

int packsMgr::reload()
{
    Query q(GetDb());
    m_packs_maps.clear();
    m_packsOpen_maps.clear();

    std::list<int> packsList;
    q.get_result("select id from custom_packs where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        packsList.push_back(q.getval());
    }
    q.free_result();

    for (std::list<int>::iterator it = packsList.begin(); it != packsList.end(); ++it)
    {
        _findPacks(*it);
    }
    return HC_SUCCESS;
}

//使用激活码，成功返回0,被使用了返回1，无效返回-1，已经领过返回2
int packsMgr::useCode(int cid, const std::string& szCode, int& packId, int& seqNo)
{
    packId = 0;
    seqNo = 0;
    //已经使用的激活码记录
    std::map<std::string, int>::iterator it = m_used_code_maps.find(szCode);
    if (it != m_used_code_maps.end())
    {
        return 1;
    }
    else
    {
        Query q(GetDb());
        q.get_result("select cid from char_opened_packs where code='" + GetDb().safestr(szCode) + "'");
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            m_used_code_maps[szCode] = q.getval();
            q.free_result();
            return 1;
        }
        else
        {
            q.free_result();

            int iSeqNo = 0;
            int iPacksId = 0;
            /* 从数据库中查询激活码对应的礼包id和批次 */
            q.get_result("select batch,tid from admin_code where uniques='" + GetDb().safestr(szCode) + "' and starttime<=unix_timestamp() and endtime>unix_timestamp()");
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                iSeqNo = q.getval();
                iPacksId = q.getval();                
            }
            q.free_result();

            if (iPacksId > 0)
            {
                boost::shared_ptr<CharOpenedPacksRecords> openPacks = _findOpenedPacks(cid);
                if (openPacks.get())
                {
                    CharOpenedPacksRecords* pOpened = openPacks.get();
                    for (std::list<_openRecords>::iterator it = pOpened->_openList.begin(); it != pOpened->_openList.end(); ++it)
                    {
                        if (it->_seqNo == iSeqNo)
                        {
                            return 2;
                        }
                    }
                    packId = iPacksId;
                    seqNo = iSeqNo;
                    return 0;
                }
                else
                {
                    packId = iPacksId;
                    seqNo = iSeqNo;
                    return 0;
                }
            }
            else
            {
                return -1;
            }
        }
    }
    return -1;
}
#if 0
//领取礼包
int packsMgr::getPacks(CharData * pc, const std::string& code, json_spirit::Object& robj)
{
    int packsId = 0, seqNo = 0;
    //使用激活码，成功返回true
    switch (useCode(pc->m_id, code, packsId, seqNo))
    {
        case 1:
            INFO("code is used <"<<code<<">"<<endl);
            return HC_ERROR_CODE_IS_USED;
        case 2:
            return HC_ERROR_ALREADY_GET_PACKS;
        case -1:
            INFO("code is invalid <"<<code<<">"<<endl);
            return HC_ERROR_CODE_IS_INVALID;
        case 0:
            break;
    }
    boost::shared_ptr<instancePacks> packs = _findPacks(packsId);
    //是否有这个礼包
    if (!packs.get() || !pc)
    {
        INFO("get packs fail, error pack id<"<<packsId<<">"<<endl);
        return HC_ERROR;
    }
    //是否已经领取过该批次礼包
    if (haveOpenedPack(pc->m_id, packsId, seqNo))
    {
        return HC_ERROR_ALREADY_GET_PACKS;
    }
    //cout<<"get packs "<<packsId<<endl;
    //给礼包东西
    giveLoots(pc, packs->_items, pc->m_area, pc->m_level, 0, NULL, &robj, true, give_packs_loot);
    //插入领取记录
    addOpenRecord(pc->m_id, packsId, code.c_str(), seqNo);
    return HC_SUCCESS;
}
#endif
//领取礼包
int packsMgr::getPacks(CharData * pc, const std::string& content, json_spirit::Object& robj)
{
    json_spirit::mValue value;
    json_spirit::read(content, value);
    //cout << "content = " << content << endl;
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
        int type = 0, id = 0, count = 0;
        READ_INT_FROM_MOBJ(type,tmp_obj,"type");
        READ_INT_FROM_MOBJ(id,tmp_obj,"id");
        READ_INT_FROM_MOBJ(count,tmp_obj,"count");
        Item item;
        item.type = type;
        item.id = id;
        item.nums = count;
        //cout << "type = " << type << " id = " << id << " count = " << count << endl;
        _items.push_back(item);
        ++it;
    }
    //给礼包东西
    std::list<Item> items = _items;
    giveLoots(pc, items, pc->m_area, pc->m_level, 0, NULL, &robj, true, give_packs_loot);
    return HC_SUCCESS;
}

//领取礼包
int packsMgr::getPacks(CharData * pc, int packsId, json_spirit::Object& robj)
{
    boost::shared_ptr<instancePacks> packs = _findPacks(packsId);
    //是否有这个礼包
    if (!packs.get() || !pc)
    {
        INFO("unknow pack id "<<packsId<<endl);
        return HC_ERROR;
    }
    //是否已经领取过
    if (haveOpenedPack(pc->m_id, packsId, 0))
    {
        return HC_ERROR_ALREADY_GET_PACKS;
    }
    if (packs->_needCode)
    {
        INFO("get pack fail,pack id="<<packsId<<",need prestige="<<packs->_prestige<<",prestige="<<pc->m_prestige<<",need level="<<packs->_level<<",level="<<pc->m_level<<endl);        
        return HC_ERROR;
    }
    if (packs->_prestige > pc->m_prestige)
    {
        INFO("get pack fail,pack id="<<packsId<<",need prestige="<<packs->_prestige<<",prestige="<<pc->m_prestige<<",need level="<<packs->_level<<",level="<<pc->m_level<<endl);        
        return HC_ERROR;
    }
    if (packs->_level > pc->m_level)
    {
        INFO("get pack fail,pack id="<<packsId<<",need prestige="<<packs->_prestige<<",prestige="<<pc->m_prestige<<",need level="<<packs->_level<<",level="<<pc->m_level<<endl);        
        return HC_ERROR;
    }
    if (packs->_vip > pc->m_vip)
    {
        INFO("get pack fail,pack id="<<packsId<<",need prestige="<<packs->_prestige<<",prestige="<<pc->m_prestige<<",need level="<<packs->_level<<",level="<<pc->m_level<<endl);        
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    //如果礼包有时间限制的
    if (!packs->_bAlwaysOpen && (time(NULL) < packs->_start_time || time(NULL) > packs->_end_time))
    {
        INFO("pack out of time , id="<<packsId<<endl);
        return HC_ERROR;
    }
    //cout<<"get packs "<<packsId<<endl;
    //给礼包东西
    std::list<Item> items = packs->_items;
    giveLoots(pc, items, pc->m_area, pc->m_level, 0, NULL, &robj, true, give_packs_loot);
    //插入领取记录
    addOpenRecord(pc->m_id, packsId);
    return HC_SUCCESS;
}

//获取未领礼包数量
int packsMgr::queryUnGetGifts(CharData * pc, json_spirit::Object &robj)
{
    int cnt = 0;
    for (std::map<int, boost::shared_ptr<instancePacks> >::iterator it = m_packs_maps.begin(); it != m_packs_maps.end(); ++it)
    {
        if (it->second.get())
        {
            instancePacks* packs = it->second.get();

            if (!packs->_bAlwaysOpen && (time(NULL) < packs->_start_time || time(NULL) > packs->_end_time))
            {
                continue;
            }

            bool bHaveOpened = haveOpenedPack(pc->m_id, packs->_id);
            //不显示需要激活码的未领取过的礼包
            if (packs->_needCode && !bHaveOpened)
            {
                ;
            }
            else
            {
                if (bHaveOpened)
                {
                    ;
                }
                else
                {
                    if (packs->_prestige > pc->m_prestige)
                    {
                        ;
                    }
                    else if (packs->_level > pc->m_level)
                    {
                        ;
                    }
                    else if (packs->_vip > pc->m_vip)
                    {
                        ;
                    }
                    else
                    {
                        ++cnt;
                    }
                }
            }
        }
    }
    robj.push_back( Pair("nums", cnt) );
    return HC_SUCCESS;
}

//礼包列表
int packsMgr::showPacks(CharData * pc, json_spirit::Object &robj)
{
    json_spirit::Array openedList;
    json_spirit::Array canOpenList;
    json_spirit::Array canNotOpenList;
    for (std::map<int, boost::shared_ptr<instancePacks> >::iterator it = m_packs_maps.begin(); it != m_packs_maps.end(); ++it)
    {
        if (it->second.get())
        {
            instancePacks* packs = it->second.get();

            if (!packs->_bAlwaysOpen && (time(NULL) < packs->_start_time || time(NULL) > packs->_end_time))
            {
                continue;
            }

            bool bHaveOpened = haveOpenedPack(pc->m_id, packs->_id);
            //不显示需要激活码的未领取过的礼包
            if (packs->_needCode && !bHaveOpened)
            {
                ;
            }
            else
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", packs->_id) );
                obj.push_back( Pair("name", packs->_name) );
                obj.push_back( Pair("memo", packs->_content) );
                if (bHaveOpened)
                {
                    obj.push_back( Pair("state", 2) );
                    openedList.push_back(obj);
                }
                else
                {
                    if (packs->_prestige > pc->m_prestige)
                    {
                        std::string reason = strPack_needMorePrestige;
                        str_replace(reason, "$P", LEX_CAST_STR(packs->_prestige));
                        obj.push_back( Pair("state", 0) );
                        obj.push_back( Pair("reason", reason) );
                        canNotOpenList.push_back(obj);
                    }
                    else if (packs->_level > pc->m_level)
                    {
                        std::string reason = strPack_needMoreLevel;
                        str_replace(reason, "$L", LEX_CAST_STR(packs->_level));
                        obj.push_back( Pair("state", 0) );
                        obj.push_back( Pair("reason", reason) );
                        canNotOpenList.push_back(obj);
                    }
                    else if (packs->_vip > pc->m_vip)
                    {
                        std::string reason = strPack_needMoreVIP;
                        str_replace(reason, "$V", LEX_CAST_STR(packs->_vip));
                        obj.push_back( Pair("state", 0) );
                        obj.push_back( Pair("reason", reason) );
                        canNotOpenList.push_back(obj);
                    }
                    else
                    {
                        obj.push_back( Pair("state", 1) );
                        canOpenList.push_back(obj);
                    }
                }
            }
        }
    }
    for (json_spirit::Array::iterator it = canNotOpenList.begin(); it != canNotOpenList.end(); ++it)
    {
        canOpenList.push_back(*it);
    }
    for (json_spirit::Array::iterator it = openedList.begin(); it != openedList.end(); ++it)
    {
        canOpenList.push_back(*it);
    }
    robj.push_back( Pair("list", canOpenList) );
    return HC_SUCCESS;
}

boost::shared_ptr<instancePacks> packsMgr::_findPacks(int packId)
{
    std::map<int, boost::shared_ptr<instancePacks> >::iterator it = m_packs_maps.find(packId);
    if (it != m_packs_maps.end())
    {
        return it->second;
    }
    return loadPacks(packId);
}

boost::shared_ptr<instancePacks> packsMgr::loadPacks(int packsId)
{
    Query q(GetDb());
    q.get_result("select ip.id,ip.pid,ip.name,ip.openYear,ip.openMonth,ip.openDay,ip.openHour,ip.openMinute,ip.closeYear,ip.closeMonth,ip.closeDay,ip.closeHour,ip.closeMinute,ip.memo,bp.vip,bp.prestige,bp.level,bp.code from custom_packs as ip left join custom_base_packs as bp on ip.pid=bp.id where ip.enable='1' and ip.id=" + LEX_CAST_STR(packsId));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        instancePacks* pIpacks = new instancePacks;
        pIpacks->_id = q.getval();
        pIpacks->_bid = q.getval();
        pIpacks->_name = q.getstr();
        pIpacks->_openYear = q.getval();
        pIpacks->_openMonth = q.getval();
        pIpacks->_openDay = q.getval();
        pIpacks->_openHour = q.getval();
        pIpacks->_openMinute = q.getval();
        pIpacks->_closeYear = q.getval();
        pIpacks->_closeMonth = q.getval();
        pIpacks->_closeDay = q.getval();
        pIpacks->_closeHour = q.getval();
        pIpacks->_closeMinute = q.getval();

        pIpacks->_memo = q.getstr();
        pIpacks->_vip = q.getval();
        pIpacks->_prestige = q.getval();
        pIpacks->_level = q.getval();
        pIpacks->_needCode = q.getval();

        if (pIpacks->_openYear == 0 && pIpacks->_openMonth == 0 && pIpacks->_openDay == 0
            && pIpacks->_openHour == 0 && pIpacks->_openMinute == 0 &&
            pIpacks->_closeYear == 0 && pIpacks->_closeMonth == 0 && pIpacks->_closeDay == 0
            && pIpacks->_closeHour == 0 && pIpacks->_closeMinute == 0)
        {
            pIpacks->_bAlwaysOpen = true;
        }
        else
        {
            pIpacks->_bAlwaysOpen = false;
            pIpacks->_start_time = spls_mktime(pIpacks->_openYear, pIpacks->_openMonth, pIpacks->_openDay, pIpacks->_openHour, pIpacks->_openMinute);
            pIpacks->_end_time = spls_mktime(pIpacks->_closeYear, pIpacks->_closeMonth, pIpacks->_closeDay, pIpacks->_closeHour, pIpacks->_closeMinute);
        }
        boost::shared_ptr<instancePacks> packs(pIpacks);
        m_packs_maps[pIpacks->_id] = packs;
        q.free_result();

        pIpacks->_content = "";
        
        q.get_result("select type,id,count from custom_base_packs_items where pid=" + LEX_CAST_STR(pIpacks->_bid));
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            pIpacks->_items.push_back(item);
            if (pIpacks->_content != "")
            {
                pIpacks->_content = pIpacks->_content + "," + item.toString();
            }
            else
            {
                pIpacks->_content = item.toString();
            }
        }
        q.free_result();

        return packs;
    }
    else
    {
        q.free_result();
        boost::shared_ptr<instancePacks> packs;
        return packs;
    }
}

boost::shared_ptr<CharOpenedPacksRecords> packsMgr::_findOpenedPacks(int cid)
{
    std::map<int, boost::shared_ptr<CharOpenedPacksRecords> >::iterator it = m_packsOpen_maps.find(cid);
    if (it != m_packsOpen_maps.end())
    {
        return it->second;
    }
    return loadOpenedPacks(cid);
}

boost::shared_ptr<CharOpenedPacksRecords> packsMgr::loadOpenedPacks(int cid)
{
    boost::shared_ptr<CharOpenedPacksRecords> openPacks;
    Query q(GetDb());
    q.get_result("select pid,seqNo,code,open_time from char_opened_packs where cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.num_rows())
    {
        openPacks.reset(new CharOpenedPacksRecords);
        openPacks->_cid = cid;
    }
    while (q.fetch_row())
    {
        _openRecords re;
        re._cid = cid;
        re._packsId = q.getval();
        re._seqNo = q.getval();
        const char* p = q.getstr();
        if (p)
        {
            strncpy(re._szCode, p, 32);
            re._szCode[32] = 0;
            m_used_code_maps[re._szCode] = cid;
        }
        else
        {
            re._szCode[0] = 0;
        }
        re._openTime = q.getval();
        openPacks->_openList.push_back(re);
    }
    q.free_result();
    m_packsOpen_maps[cid] = openPacks;
    return openPacks;
}

bool packsMgr::haveOpenedPack(int cid, int packid)
{
    boost::shared_ptr<CharOpenedPacksRecords> openPacks = _findOpenedPacks(cid);
    if (!openPacks.get())
    {
        return false;
    }
    CharOpenedPacksRecords* pOpened = openPacks.get();
    for (std::list<_openRecords>::iterator it = pOpened->_openList.begin(); it != pOpened->_openList.end(); ++it)
    {
        if (it->_packsId == packid)
        {
            return true;
        }
    }
    return false;
}

bool packsMgr::haveOpenedPack(int cid, int packid, int seqNo)
{
    boost::shared_ptr<CharOpenedPacksRecords> openPacks = _findOpenedPacks(cid);
    if (!openPacks.get())
    {
        return false;
    }
    CharOpenedPacksRecords* pOpened = openPacks.get();
    for (std::list<_openRecords>::iterator it = pOpened->_openList.begin(); it != pOpened->_openList.end(); ++it)
    {
        if (it->_packsId == packid && it->_seqNo == seqNo)
        {
            return true;
        }
    }
    return false;
}

void packsMgr::addOpenRecord(int cid, int packsId)
{
    boost::shared_ptr<CharOpenedPacksRecords> openPacks = _findOpenedPacks(cid);
    if (!openPacks.get())
    {
        openPacks.reset(new CharOpenedPacksRecords);
        openPacks->_cid = cid;
        m_packsOpen_maps[cid] = openPacks;
    }
    CharOpenedPacksRecords* pOpened = openPacks.get();
    _openRecords re;
    re._cid = cid;
    re._packsId = packsId;
    re._openTime = time(NULL);
    re._seqNo = 0;
    re._szCode[0] = 0;
    pOpened->_openList.push_back(re);
    InsertSaveDb("insert into char_opened_packs (cid,pid,open_time) values ("
        + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(packsId) + "," + LEX_CAST_STR(re._openTime) + ")");
}

void packsMgr::addOpenRecord(int cid, int packsId, const char* szCode, int seqNo)
{
    boost::shared_ptr<CharOpenedPacksRecords> openPacks = _findOpenedPacks(cid);
    if (!openPacks.get())
    {
        openPacks.reset(new CharOpenedPacksRecords);
        openPacks->_cid = cid;
        m_packsOpen_maps[cid] = openPacks;
    }
    CharOpenedPacksRecords* pOpened = openPacks.get();
    _openRecords re;
    re._cid = cid;
    re._packsId = packsId;
    re._openTime = time(NULL);
    re._seqNo = seqNo;
    strncpy(re._szCode, szCode, 32);
    pOpened->_openList.push_back(re);
    InsertSaveDb("insert into char_opened_packs (cid,pid,open_time,code,seqNo) values ("
        + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(packsId) + "," + LEX_CAST_STR(re._openTime)
        + ",'" + GetDb().safestr(re._szCode) + "'," + LEX_CAST_STR(seqNo) + ")");

    m_used_code_maps[re._szCode] = cid;
}

//领取记录导出
void packsMgr::export_opened(int cid, json_spirit::Array& a)
{
    boost::shared_ptr<CharOpenedPacksRecords> openPacks = _findOpenedPacks(cid);
    if (!openPacks.get())
    {
        return;
    }
    CharOpenedPacksRecords* pOpened = openPacks.get();
    for (std::list<_openRecords>::iterator it = pOpened->_openList.begin(); it != pOpened->_openList.end(); ++it)
    {
        boost::shared_ptr<instancePacks> packs = _findPacks(it->_packsId);
        if (packs.get())
        {
            json_spirit::Object p;
            p.push_back( Pair("packName", packs->_name) );
            p.push_back( Pair("code", it->_szCode) );
            p.push_back( Pair("seqNo", it->_seqNo) );
            p.push_back( Pair("open_time", it->_openTime) );
            a.push_back(p);
        }
    }
    return;
}

int packsMgr::deleteChar(int cid)
{
    InsertSaveDb("delete from char_opened_packs where cid=" + LEX_CAST_STR(cid));
    m_packsOpen_maps.erase(cid);
    return HC_SUCCESS;
}

//cmd:getGiftList//获得礼品界面信息
int ProcessListPacks(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return packsMgr::getInstance()->showPacks(cdata.get(), robj);
}

int ProcessQueryUnGetGifts(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return packsMgr::getInstance()->queryUnGetGifts(cdata.get(), robj);
}

