
#include "bank.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "net.h"
#include "statistics.h"

using namespace net;

#define INFO(x)// cout<<x
extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);

void char_bank_case::save()
{
    std::string sql = "update char_bank_cases set state=" + LEX_CAST_STR(m_state) +
        ",get=" + LEX_CAST_STR(m_get_gold) + ",start_time=" + LEX_CAST_STR(m_start_time);
    for (size_t i = 0; i < iBankSubCaseCount; ++i)
    {
        sql += (",feedback_state" + LEX_CAST_STR(i+1) + "=" + LEX_CAST_STR(m_feedback_state[i]));
    }
    sql += " where cid=" + LEX_CAST_STR(m_cid) + " and case_id=" + LEX_CAST_STR(m_case_id);
    InsertSaveDb(sql);
    return;
}

static bool bankUnGet(char_bank_list& b)
{
    for (size_t i = 0; i < b.size(); ++i)
    {
        char_bank_case* bc = b[i].get();
        if (bc)
        {
            if (bc->m_state == 2)
            {
                return true;
            }
        }
    }
    return false;
}

static int bankCanGet(char_bank_list& b)
{
    int ret = 2;
    for (size_t i = 0; i < b.size(); ++i)
    {
        boost::shared_ptr<char_bank_case> bc = b[i];
        if (bc.get())
        {
            for (size_t j = 0; j < iBankSubCaseCount; ++j)
            {
                if (bc->m_feedback_state[j] == 1)
                {
                    return 1;
                }
                else if (bc->m_feedback_state[j] == 0)
                {
                    ret = 0;
                }
            }
        }
    }
    return ret;
}

static int bankCanGet(char_bank_list& b, int w)
{
    int ret = 2;
    for (size_t i = 0; i < b.size(); ++i)
    {
        boost::shared_ptr<char_bank_case> bc = b[i];
        if (bc.get())
        {
            if (bc->m_case_id == w)
            {
                for (size_t j = 0; j < iBankSubCaseCount; ++j)
                {
                    if (bc->m_feedback_state[j] == 1)
                    {
                        return 1;
                    }
                    else if (bc->m_feedback_state[j] == 0)
                    {
                        ret = 0;
                    }
                }
            }
        }
    }
    return ret;
}


bankMgr* bankMgr::m_handle = NULL;

bankMgr* bankMgr::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"bankMgr::getInstance()..."<<endl;
        m_handle = new bankMgr();
        m_handle->reload();
        cout<<"bankMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

void bankMgr::reload()
{
    Query q(GetDb());
    //load all activity farm_field
    std::list<int> id_list;
    q.get_result("select id,need_gold from base_bank_cases where 1 order by id");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<bank_base_case> p_b;
        p_b.reset(new bank_base_case);
        p_b->_id = q.getval();
        p_b->_need_gold = q.getval();
        p_b->_total_feedback_gold = 0;
        m_base_banks[p_b->_id] = p_b;
        m_max_case_id = p_b->_id;
    }
    q.free_result();
    std::map<int, boost::shared_ptr<bank_base_case> >::iterator it = m_base_banks.begin();
    while (it != m_base_banks.end() && it->second.get())
    {
        int idx = 0;
        q.get_result("select wait_mins,gold from base_bank_cases_info where id=" + LEX_CAST_STR(it->second->_id) + " order by wait_mins limit 4");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            bank_sub_case& sc = it->second->_case_map[idx];
            sc._need_mins = q.getval();
            sc._feedback_gold = q.getval();
            it->second->_total_feedback_gold += sc._feedback_gold;
            ++idx;
        }
        q.free_result();
        ++it;
    }

    m_start_time = GeneralDataMgr::getInstance()->getInt("bank_event_start", 0);
    m_end_time = GeneralDataMgr::getInstance()->getInt("bank_event_end", 0);

    if (m_start_time == 0)
    {
        struct tm tm;
        struct tm *t = &tm;
        time_t time_now = time(NULL);
        localtime_r(&time_now, t);
        t->tm_hour = 0;
        t->tm_min = 0;
        t->tm_sec = 0;
        m_start_time = mktime(t);
        m_end_time = m_start_time + 30 * iONE_DAY_SECS;
        GeneralDataMgr::getInstance()->setInt("bank_event_start", m_start_time);
        GeneralDataMgr::getInstance()->setInt("bank_event_end", m_end_time);
    }
    return;
}

boost::shared_ptr<bank_base_case> bankMgr::GetBaseCase(int case_id)
{
    INFO( "get base case:" << case_id << endl);
    std::map<int, boost::shared_ptr<bank_base_case> >::iterator it = m_base_banks.find(case_id);
    if (it != m_base_banks.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<bank_base_case> p;
    p.reset();
    return p;
}

boost::shared_ptr<char_bank_list> bankMgr::GetCharBankList(int cid)
{
    boost::shared_ptr<char_bank_list> p_cb;
    std::map<int, boost::shared_ptr<char_bank_list> >::iterator it =  m_char_banks.find(cid);
    if (it != m_char_banks.end())
    {
        return it->second;
    }
    else
    {
        p_cb.reset(new char_bank_list);
        INFO("load charbanklist from db" << endl);
        Query q(GetDb());
        q.get_result("select state,case_id,get,feedback_state1,feedback_state2,feedback_state3,feedback_state4,start_time from char_bank_cases where cid=" + LEX_CAST_STR(cid) + " order by case_id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            boost::shared_ptr<char_bank_case> p_c;
            p_c.reset(new char_bank_case);
            p_c->m_timer = boost::uuids::nil_uuid();
            p_c->m_cid = cid;
            int state = q.getval();
            if (state == 0)
            {
                state = 1;
            }
            p_c->m_state = state;
            int case_id = q.getval();
            p_c->m_case_id = case_id;
            p_c->m_base = GetBaseCase(case_id);
            
            p_c->m_get_gold = q.getval();

            if (state == 2)
            {
                //vector
                for (int i = 0; i < iBankSubCaseCount; ++i)
                {
                    p_c->m_feedback_state[i] = q.getval();
                }
                p_c->m_start_time = q.getval();
                if (p_c->m_start_time == 0)
                {
                    p_c->m_start_time = time(NULL);
                }
                if (p_c->m_base.get())
                {
                    bank_base_case* b = p_c->m_base.get();
                    for (int i = 0; i < iBankSubCaseCount; ++i)
                    {
                        if (p_c->m_feedback_state[i] == 0)
                        {
                            p_c->m_sub_can_get_time[i] = p_c->m_start_time + b->_case_map[i]._need_mins * 60;
                            if (p_c->m_sub_can_get_time[i] <= time(NULL))
                            {
                                p_c->m_feedback_state[i] = 1;
                                p_c->m_sub_can_get_time[i] = 0;
                            }
                            else
                            {
                                //开启定时器
                                if (p_c->m_timer.is_nil())
                                {
                                    //定时器通知，刷新
                                    json_spirit::mObject mobj;
                                    mobj["cmd"] = "bankCanGet";
                                    mobj["cid"] = cid;
                                    mobj["case"] = p_c->m_case_id;
                                    mobj["sid"] = (i+1);
                                    boost::shared_ptr<splsTimer> tmsg;
                                    tmsg.reset(new splsTimer(p_c->m_sub_can_get_time[i]-time(NULL), 1, mobj,1));
                                    p_c->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
                                }
                            }
                        }
                    }
                }
            }
            (*p_cb).push_back(p_c);
        }
        q.free_result();
        m_char_banks[cid] = p_cb;
        if (p_cb->size() == 0)
        {
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (!cdata.get())
            {                
                return p_cb;
            }
            if (isOpen())
            {
                return open(cid);
            }
        }
        INFO("load charbanklist success" << endl);
        return p_cb;
    }
    p_cb.reset();
    return p_cb;
}

boost::shared_ptr<char_bank_list> bankMgr::open(int cid)
{
    std::map<int, boost::shared_ptr<char_bank_list> >::iterator it =  m_char_banks.find(cid);
    if (it != m_char_banks.end() && it->second->size() == m_max_case_id)
    {
        return it->second;
    }
    boost::shared_ptr<char_bank_list> p_cb;
    p_cb.reset(new char_bank_list);
    //钱庄
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return p_cb;
    for (int i = 1; i <= m_max_case_id; ++i)
    {
        boost::shared_ptr<char_bank_case> p_c;
        p_c.reset(new char_bank_case);
        p_c->m_base = GetBaseCase(i);
        if (p_c->m_base.get())
        {
            p_c->m_cid = cid;
            p_c->m_case_id = i;
            p_c->m_state = 1;
            p_c->m_get_gold = 0;
            p_c->m_start_time = 0;
            p_c->m_timer = boost::uuids::nil_uuid();
            (*p_cb).push_back(p_c);
            InsertSaveDb("insert into char_bank_cases (cid,case_id,state,start_time) values ("
                + LEX_CAST_STR(cid) + ","
                + LEX_CAST_STR(i) + ","
                + LEX_CAST_STR(p_c->m_state) + ","
                + LEX_CAST_STR(p_c->m_start_time) +")");
        }
        else
        {
            p_c.reset();
        }
    }
    m_char_banks[cid] = p_cb;
    if (bankCanGet(*p_cb) == 1 || cdata->queryExtraData(char_data_type_normal, char_data_view_bank) == 0)
    {
        //按钮闪动
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
        if (account.get())
        {
            account->Send("{\"cmd\":\"updateAction\",\"type\":13,\"active\":1,\"s\":200}");
        }
    }
    return p_cb;
}

void bankMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    if (pc->m_bankOpen)
    {        
        boost::shared_ptr<char_bank_list> bk = GetCharBankList(pc->m_id);
        if (bk.get())
        {
            if (isOpen() || bankUnGet(*bk))
            {
                int state = bankCanGet(*bk);
                if (state != 2)
                {
                    //第一闪
                    if (state == 0 && pc->queryExtraData(char_data_type_normal, char_data_view_bank) == 0)
                    {
                        state = 1;
                    }
                    json_spirit::Object obj;
                    obj.push_back( Pair("type", top_level_event_bank) );
                    obj.push_back( Pair("active", state) );
                    blist.push_back(obj);
                }
            }
        }
    }
}

bool bankMgr::isOpen()
{
    time_t timenow = time(NULL);
    return timenow >= m_start_time && timenow < m_end_time;
}

int ProcessBuyBankCase(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_bankOpen == 0 || bankMgr::getInstance()->isOpen() == false)
    {
        return HC_ERROR;
    }
    int case_id = 1;
    READ_INT_FROM_MOBJ(case_id, o, "case_id");
    json_spirit::Array list;
    boost::shared_ptr<char_bank_list> p_cb = bankMgr::getInstance()->GetCharBankList(cdata->m_id);
    if (p_cb.get())
    {
        for (int i = 0; i < (int)(*p_cb).size(); ++i)
        {
            //项目可投资
            if ((*p_cb)[i].get() && (*p_cb)[i]->m_case_id == case_id && (*p_cb)[i]->m_state == 1)
            {
                if((*p_cb)[i]->m_base.get())
                {
                    if (cdata->addGold(-(*p_cb)[i]->m_base->_need_gold) < 0)
                        return HC_ERROR_NOT_ENOUGH_GOLD;
                    cdata->NotifyCharData();
                    //金币消耗统计
                    {
                        int tmp_type = gold_cost_for_bank1;
                        switch(case_id)
                        {
                            case 1:
                                tmp_type = gold_cost_for_bank1;
                                break;
                            case 2:
                                tmp_type = gold_cost_for_bank2;
                                break;
                            case 3:
                                tmp_type = gold_cost_for_bank3;
                                break;
                            case 4:
                                tmp_type = gold_cost_for_bank4;
                                break;
                        }
                        //细分统计
                        add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, (*p_cb)[i]->m_base->_need_gold, tmp_type, cdata->m_union_id, cdata->m_server_id);
                        //总统计
                        //add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, (*p_cb)[i]->m_base->_need_gold, gold_cost_for_bank, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
                        gold_cost_tencent(cdata,(*p_cb)[i]->m_base->_need_gold,tmp_type);
                        gold_cost_tencent(cdata,(*p_cb)[i]->m_base->_need_gold,gold_cost_for_bank);
#endif
                    }
                    //act统计
                    act_to_tencent(cdata,act_new_bank,case_id);
                    //投资
                    (*p_cb)[i]->m_state = 2;
                    (*p_cb)[i]->m_start_time = time(NULL);
                    for (int j = 0; j < iBankSubCaseCount; ++j)
                    {
                        if ((*p_cb)[i]->m_base->_case_map[j]._need_mins == 0)
                        {
                            //可以直接领取
                            (*p_cb)[i]->m_feedback_state[j] = 1;
                        }
                        else
                        {
                            (*p_cb)[i]->m_sub_can_get_time[j] = (*p_cb)[i]->m_start_time + (*p_cb)[i]->m_base->_case_map[j]._need_mins * 60;
                            //开启定时器
                            if ((*p_cb)[i]->m_timer.is_nil())
                            {
                                //定时器通知，刷新
                                json_spirit::mObject mobj;
                                mobj["cmd"] = "bankCanGet";
                                mobj["cid"] = cdata->m_id;
                                mobj["case"] = (*p_cb)[i]->m_case_id;
                                mobj["sid"] = (j+1);
                                boost::shared_ptr<splsTimer> tmsg;
                                tmsg.reset(new splsTimer((*p_cb)[i]->m_sub_can_get_time[j]-time(NULL), 1, mobj,1));
                                (*p_cb)[i]->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);
                            }
                        }
                    }
                    if (bankCanGet(*p_cb) == 1)
                    {
                        //按钮闪动
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                        if (account.get())
                        {
                            account->Send("{\"cmd\":\"updateAction\",\"type\":13,\"active\":1,\"s\":200}");
                        }
                    }
                    (*p_cb)[i]->save();
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

int ProcessGetBankFeedback(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_bankOpen == 0)
    {
        return HC_ERROR;
    }
    int case_id = 1, sid = 0;
    READ_INT_FROM_MOBJ(case_id, o, "case_id");
    READ_INT_FROM_MOBJ(sid, o, "sid");
    if (sid < 1 || sid > iBankSubCaseCount)
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    boost::shared_ptr<char_bank_list> p_cb = bankMgr::getInstance()->GetCharBankList(cdata->m_id);
    if (p_cb.get())
    {
        if (bankMgr::getInstance()->isOpen() == false && bankUnGet(*p_cb) == false)
        {
            return HC_ERROR;
        }
        for (int i = 0; i < (int)(*p_cb).size(); ++i)
        {
            //项目已经投资
            if ((*p_cb)[i].get() && (*p_cb)[i]->m_case_id == case_id && (*p_cb)[i]->m_state == 2)
            {
                if((*p_cb)[i]->m_base.get())
                {
                    char_bank_case* pb = (*p_cb)[i].get();
                    {
                        if (pb->m_feedback_state[sid-1] == 1)
                        {
                            int gold = pb->m_base->_case_map[sid-1]._feedback_gold;
                            cdata->addGold(gold);
                            cdata->NotifyCharData();
                            //金币获得统计
                            {
                                switch(case_id)
                                {
                                    case 1:
                                        add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address, gold, gold_get_bank1, cdata->m_union_id, cdata->m_server_id);
                                        break;
                                    case 2:
                                        add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address, gold, gold_get_bank2, cdata->m_union_id, cdata->m_server_id);
                                        break;
                                    case 3:
                                        add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address, gold, gold_get_bank3, cdata->m_union_id, cdata->m_server_id);
                                        break;
                                    case 4:
                                        add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address, gold, gold_get_bank4, cdata->m_union_id, cdata->m_server_id);
                                        break;
                                }
                                add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address, gold, gold_get_bank, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
                                gold_get_tencent(cdata, gold);
#endif
                            }
                            (*p_cb)[i]->m_get_gold += gold;
                            (*p_cb)[i]->m_feedback_state[sid-1] = 2;
                            (*p_cb)[i]->save();
                            int state = bankCanGet(*p_cb);
                            if (0 == state)
                            {
                                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                                if (account.get())
                                {
                                    account->Send("{\"cmd\":\"updateAction\",\"type\":13,\"active\":0,\"s\":200}");
                                }
                            }
                            else if (2 == state)
                            {
                                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                                if (account.get())
                                {
                                    account->Send("{\"cmd\":\"removeAction\",\"type\":13,\"s\":200}");
                                }
                            }
                            robj.push_back( Pair("gold", gold) );
                            return HC_SUCCESS;
                        }
                        else
                        {
                            return HC_ERROR;
                        }
                    }
                }
            }
        }
    }
    return HC_ERROR;
}

int ProcessGetCaseInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_bankOpen == 0)
    {
        return HC_ERROR;
    }
    int case_id = 1;
    READ_INT_FROM_MOBJ(case_id, o, "case_id");
    json_spirit::Array list;
    boost::shared_ptr<char_bank_list> p_cb = bankMgr::getInstance()->GetCharBankList(cdata->m_id);
    if (p_cb.get())
    {
        if (bankMgr::getInstance()->isOpen() == false && bankUnGet(*p_cb) == false)
        {
            return HC_ERROR;
        }
        for (int i = 0; i < (int)(*p_cb).size(); ++i)
        {
            if ((*p_cb)[i].get() && (*p_cb)[i]->m_case_id == case_id)
            {
                if((*p_cb)[i]->m_base.get())
                {
                    time_t stime = 0;
                    if ((*p_cb)[i]->m_state == 2)
                    {
                        stime = time(NULL);
                    }

                    bool btmp = false;
                    for (size_t s = 0; s < iBankSubCaseCount; ++s)
                    {
                        json_spirit::Object o;
                        o.push_back( Pair("sid", s+1) );
                        o.push_back( Pair("secs", (*p_cb)[i]->m_base->_case_map[s]._need_mins*60) );
                        o.push_back( Pair("gold", (*p_cb)[i]->m_base->_case_map[s]._feedback_gold) );
                        o.push_back( Pair("state", (*p_cb)[i]->m_feedback_state[s]) );
                        if ((*p_cb)[i]->m_feedback_state[s] == 0 && btmp == false)
                        {
                            int left = (*p_cb)[i]->m_sub_can_get_time[s] - stime;
                            if (left > 0)
                            {
                                o.push_back( Pair("left", left) );
                                btmp = true;
                            }
                        }
                        list.push_back(o);
                    }
                }
                robj.push_back( Pair("caseInfo", list) );
                robj.push_back( Pair("state", (*p_cb)[i]->m_state) );
                break;
            }
            else
            {
                INFO("case_id=" << (*p_cb)[i]->m_case_id << endl);
            }
        }
    }
    return HC_SUCCESS;
}

int ProcessGetBankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    if (cdata->m_bankOpen == 0)
    {
        return HC_ERROR;
    }
    json_spirit::Array bank_cases_list;
    boost::shared_ptr<char_bank_list> p_cb = bankMgr::getInstance()->GetCharBankList(cdata->m_id);
    if (p_cb.get())
    {
        if (bankMgr::getInstance()->isOpen() == false && bankUnGet(*p_cb) == false)
        {
            return HC_ERROR;
        }
        for (int i = 0; i < (int)(*p_cb).size(); ++i)
        {
            if ((*p_cb)[i].get())
            {
                json_spirit::Object o;
                if((*p_cb)[i]->m_base.get())
                {
                    o.push_back( Pair("id", (*p_cb)[i]->m_base->_id) );
                    o.push_back( Pair("need_gold", (*p_cb)[i]->m_base->_need_gold) );
                    o.push_back( Pair("feedback_gold", (*p_cb)[i]->m_base->_total_feedback_gold) );
                }
                int can_get = bankCanGet((*p_cb), i+1);
                if (can_get == 1)
                {
                    o.push_back( Pair("can_get", true) );
                }
                else
                {
                    o.push_back( Pair("can_get", false) );
                }
                o.push_back( Pair("get", (*p_cb)[i]->m_get_gold) );
                o.push_back( Pair("state", (*p_cb)[i]->m_state) );
                bank_cases_list.push_back(o);
            }
        }
    }
    robj.push_back( Pair("caseList", bank_cases_list) );
    robj.push_back( Pair("starttime", bankMgr::getInstance()->getStart() ) );
    robj.push_back( Pair("endtime", bankMgr::getInstance()->getEnd() ) );
    //第一闪
    int first_view = cdata->queryExtraData(char_data_type_normal, char_data_view_bank);
    if (0 == first_view)
    {
        cdata->setExtraData(char_data_type_normal, char_data_view_bank, 1);
        if (p_cb.get())
        {
            int state = bankCanGet(*p_cb);
            if (0 == state)
            {
                cdata->notifyEventState(top_level_event_bank, 0, 0);
            }
        }
    }
    return HC_SUCCESS;
}

//投资可领取
int ProcessBankCaseCanGet(json_spirit::mObject& o)
{
    int cid = 0, case_id = 0, sid = 0;
    READ_INT_FROM_MOBJ(cid,o,"cid");
    READ_INT_FROM_MOBJ(case_id,o,"case");
    READ_INT_FROM_MOBJ(sid,o,"sid");
    if (sid < 1 || sid > iBankSubCaseCount)
    {
        cout<<cid<<","<<case_id<<","<<sid<<endl;
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<char_bank_list> p_cb = bankMgr::getInstance()->GetCharBankList(cid);
    if (p_cb.get())
    {
        for (int i = 0; i < (int)(*p_cb).size(); ++i)
        {
            //项目已经投资
            if ((*p_cb)[i].get() && (*p_cb)[i]->m_case_id == case_id && (*p_cb)[i]->m_state == 2)
            {
                if((*p_cb)[i]->m_base.get())
                {
                    char_bank_case* pb = (*p_cb)[i].get();
                    {
                        if (pb->m_feedback_state[sid-1] == 0)
                        {
                            pb->m_feedback_state[sid-1] = 1;
                            pb->m_sub_can_get_time[sid-1] = 0;
                            if (sid+1 <= iBankSubCaseCount && pb->m_feedback_state[sid] == 0)
                            {
                                //定时器通知，刷新
                                json_spirit::mObject mobj;
                                mobj["cmd"] = "bankCanGet";
                                mobj["cid"] = cid;
                                mobj["case"] = case_id;
                                mobj["sid"] = (sid+1);
                                boost::shared_ptr<splsTimer> tmsg;
                                tmsg.reset(new splsTimer(pb->m_sub_can_get_time[sid]-time(NULL), 1, mobj,1));
                                pb->m_timer = splsTimerMgr::getInstance()->addTimer(tmsg);        
                            }
                            else
                            {
                                pb->m_timer = boost::uuids::nil_uuid();
                            }
                            //按钮闪动
                            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
                            if (cdata.get())
                            {
                                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
                                if (account.get())
                                {
                                    account->Send("{\"cmd\":\"updateAction\",\"type\":13,\"active\":1,\"s\":200}");
                                }
                            }
                        }
                    }
                }
                else
                {
                    cout<<cid<<","<<case_id<<","<<sid<<endl;
                    ERR();
                }
                return HC_SUCCESS;
            }            
        }
    }
    else
    {
        cout<<cid<<","<<case_id<<","<<sid<<endl;
        ERR();
    }
    return HC_SUCCESS;
}

