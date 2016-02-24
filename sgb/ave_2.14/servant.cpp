#include "statistics.h"

#include "servant.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_timer.h"
#include "spls_const.h"
#include "spls_race.h"
#include "daily_task.h"
#include "singleton.h"
#include "relation.h"

class Combat;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);
extern Combat* createServantCombat(int cid, int tid, int extra_data, int extra_data2, int& ret);


//static     boost::uuids::nil_generator gen;

extern std::string strServantCatchSucAtk;
extern std::string strServantCatchSucDef;
extern std::string strServantCatchFailAtk;
extern std::string strServantCatchFailDef;
extern std::string strServantRobSucAtk;
extern std::string strServantRobSucDef;
extern std::string strServantRobFailAtk;
extern std::string strServantRobFailDef;
extern std::string strServantRedeemSucAtk;
extern std::string strServantRedeemSucDef;
extern std::string strServantResistSucAtk;
extern std::string strServantResistSucDef;
extern std::string strServantResistFailAtk;
extern std::string strServantResistFailDef;
extern std::string strServantSOSSucAtk;
extern std::string strServantSOSSucDef;
extern std::string strServantSOSSucHelper;
extern std::string strServantSOSFailAtk;
extern std::string strServantSOSFailDef;
extern std::string strServantSOSFailHelper;
extern std::string strServantReleaseSucAtk;
extern std::string strServantReleaseSucDef;
extern std::string strServantDoneSucAtk;
extern std::string strServantDoneSucDef;
extern std::string strServantGetMsg;

//壮丁玉石产出每小时
int getServantOutput(int cid)
{
    int output = 0;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        output = cdata->m_level * 3 + 50;
        //家丁收益系数
        servantMgr::getInstance()->servantRealReward(output);
    }
    return output;
}

inline int getInteractJade(int level)
{
    int output = level * 10 + 520;
    //家丁收益系数
    servantMgr::getInstance()->servantRealReward(output);
    return output;
}

int charServant::start()
{
    //cout << "charServant cid=" << m_cid << ",start" << endl;
    //重启后的情况
    int leftsecond = m_end_time - time(NULL);
    int now_left = leftsecond / iServantTime + (leftsecond % iServantTime > 0 ? 1 : 0);

    json_spirit::mObject mobj;
    mobj["cmd"] = "servantDone";
    mobj["cid"] = m_cid;
    boost::shared_ptr<splsTimer> tmsg;
    if (leftsecond <= 0 || m_left > now_left)
    {
        //直接完成了
        tmsg.reset(new splsTimer(0.1, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    else
    {
        int start_first_time = leftsecond % iServantTime;
        if (start_first_time == 0)
            start_first_time += iServantTime;
        tmsg.reset(new splsTimer(start_first_time, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return 0;
}

int charServant::done()
{
    //cout << "charServant cid=" << m_cid << ",done" << endl;
    int get = 0;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
        return HC_ERROR;
    if (m_left > 0)
    {
        m_output += getServantOutput(m_cid)/6;
        --m_left;
    }
    if (m_left <= 0)//全部结束
    {
        boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(m_master_id);
        boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(m_master_id);
        if (pcd.get() && p.get())
        {
            int max = servantMgr::getInstance()->getMax(*pcd);
            if ((p->m_get_num + m_output) >= max)
            {
                m_output = max - p->m_get_num;                
            }

            if (m_output > 0)
            {
                get = m_output;
                pcd->addTreasure(treasure_type_yushi, m_output);
#ifdef QQ_PLAT
                gold_cost_tencent(pcd.get(),0,gold_cost_for_convert_jade,treasure_type_yushi,m_output);
#endif
                add_statistics_of_treasure_cost(pcd->m_id, pcd->m_ip_address, treasure_type_yushi, m_output, treasure_servant, 1, pcd->m_union_id,pcd->m_server_id);
                p->m_get_num += m_output;
            }
            std::string msg = strServantDoneSucAtk;
            str_replace(msg, "$N", cdata->m_name);
            str_replace(msg, "$N", cdata->m_name,true);
            servantMgr::getInstance()->addServantEvent(pcd->m_id, msg);
            msg = strServantDoneSucDef;
            str_replace(msg, "$N", pcd->m_name);
            str_replace(msg, "$N", pcd->m_name,true);
            servantMgr::getInstance()->addServantEvent(cdata->m_id, msg);
            servantMgr::getInstance()->Save_data(m_master_id);
        }
        stop();
    }
    else
    {
        start();
        servantMgr::getInstance()->Save_data(m_cid);
    }
    return get;
}

int charServant::stop()
{
    //cout << "charServant cid=" << m_cid << ",stop" << endl;
    if (m_start_time != 0 && m_end_time != 0)
    {
        m_left = 0;
        m_output = 0;
        m_type = type_free;
        m_start_time = 0;
        m_end_time = 0;
        servantMgr::getInstance()->RemoveServant(m_master_id,m_cid);
        boost::shared_ptr<charServant> p = servantMgr::getInstance()->GetCharServant(m_master_id);
        if (p.get())
        {
            if (p->m_type == type_master && p->m_servant_list.empty())
            {
                p->m_type = type_free;
            }
            servantMgr::getInstance()->Save_data(m_master_id);
        }
        m_master_id = 0;
        splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
        servantMgr::getInstance()->Save_data(m_cid);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

servantMgr* servantMgr::m_handle = NULL;

servantMgr* servantMgr::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"servantMgr::getInstance()..."<<endl;
        m_handle = new servantMgr();
        m_handle->reload();
        cout<<"servantMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

int servantMgr::getAction(CharData* cdata, json_spirit::Array& elist)
{
    if (cdata->m_servantOpen)
    {
        json_spirit::Object obj;
        obj.clear();
        obj.push_back( Pair("type", top_level_event_servant) );
        obj.push_back( Pair("active", 0) );
        boost::shared_ptr<charServant> p = GetCharServant(cdata->m_id);
        if (p.get())
        {
            obj.push_back( Pair("leftNums", iServantCatchTime + p->m_buy_catch_time - p->m_catch_time) );
        }
        elist.push_back(obj);
        return 1;
    }
    return 0;
}

void servantMgr::servantRealReward(int &get)
{
    if (m_servant_factor > 100)
    {
        get = get * m_servant_factor / 100;
    }
}

int servantMgr::reload()
{
    m_servant_factor = GeneralDataMgr::getInstance()->getInt("servant_event", 100);
    if (m_servant_factor < 100)
    {
        m_servant_factor = 100;
    }
    
    Query q(GetDb());

    q.get_result("SELECT id,type,name,memo,memo2 FROM base_servant_interact WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<baseInteract> pbi;
        pbi.reset(new baseInteract);
        pbi->m_id = q.getval();
        pbi->m_type = q.getval();
        pbi->m_name = q.getstr();
        pbi->m_memo = q.getstr();
        pbi->m_memo2 = q.getstr();
        m_base_Interact_list.push_back(pbi);
    }
    q.free_result();

    //删除错误数据
    q.execute("delete from char_servant where cid not in (select id from charactors)");
    CHECK_DB_ERR(q);
    q.execute("delete from char_servant where master_id>0 and master_id not in (select id from charactors)");
    CHECK_DB_ERR(q);
    q.execute("delete from char_servant_enemy where cid not in (select id from charactors) or enemyid not in (select id from charactors)");
    CHECK_DB_ERR(q);
    q.execute("delete from char_servant_loser where cid not in (select id from charactors) or loserid not in (select id from charactors)");
    CHECK_DB_ERR(q);

    q.get_result("SELECT cid,ctype,catch_time,buy_catch_time,interact_time,rescue_time,sos_time,be_sos_time_f,be_sos_time_c,resist_time,buy_resist_time,interact_cooltime,master_id,output,left_time,start_time,end_time,get_num FROM char_servant WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<charServant> pcs;
        pcs.reset(new charServant);
        int id = q.getval();
        pcs->m_cid = id;
        pcs->m_type = q.getval();
        pcs->m_catch_time = q.getval();
        pcs->m_buy_catch_time = q.getval();
        pcs->m_interact_time = q.getval();
        pcs->m_rescue_time = q.getval();
        pcs->m_sos_time = q.getval();
        pcs->m_be_sos_time_f = q.getval();
        pcs->m_be_sos_time_c = q.getval();
        pcs->m_resist_time = q.getval();
        pcs->m_buy_resist_time = q.getval();
        pcs->m_interact_cooltime = q.getval();
        pcs->m_master_id = q.getval();
        pcs->m_output = q.getval();
        pcs->m_left = q.getval();
        pcs->m_start_time = q.getval();
        pcs->m_end_time = q.getval();
        pcs->m_get_num = q.getval();
        Query new_q(GetDb());
        new_q.get_result("SELECT cid FROM char_servant WHERE ctype = 2 and master_id=" + LEX_CAST_STR(pcs->m_cid));
        CHECK_DB_ERR(new_q);
        while(new_q.fetch_row())
        {
            pcs->m_servant_list.push_back(new_q.getval());
        }
        new_q.free_result();
        if (pcs->m_servant_list.empty() && pcs->m_type == type_master)
        {
            pcs->m_type = type_free;
        }
        new_q.get_result("SELECT enemyid FROM char_servant_enemy WHERE cid=" + LEX_CAST_STR(pcs->m_cid));
        CHECK_DB_ERR(new_q);
        while(new_q.fetch_row())
        {
            pcs->m_enemy_list.push_back(new_q.getval());
        }
        new_q.free_result();
        new_q.get_result("SELECT loserid FROM char_servant_loser WHERE cid=" + LEX_CAST_STR(pcs->m_cid));
        CHECK_DB_ERR(new_q);
        while(new_q.fetch_row())
        {
            pcs->m_loser_list.push_back(new_q.getval());
        }
        new_q.free_result();
        new_q.get_result("SELECT event FROM char_servant_event WHERE cid=" + LEX_CAST_STR(pcs->m_cid));
        CHECK_DB_ERR(new_q);
        while(new_q.fetch_row())
        {
            pcs->m_event_list.push_back(new_q.getstr());
        }
        new_q.free_result();
        m_charServant_list[id] = pcs;
    }
    q.free_result();
    std::map<uint64_t, boost::shared_ptr<charServant> >::iterator it = m_charServant_list.begin();
    while (it != m_charServant_list.end())
    {
        if (it->second->m_type == type_servant)
            it->second->start();
        else
            it->second->m_master_id = 0;
        ++it;
    }
    return 0;
}

int servantMgr::Save_data(int cid)
{
    //cout << "servantMgr::Save_data cid=" << cid << endl;
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        InsertSaveDb("update char_servant set ctype=" + LEX_CAST_STR(p->m_type)
            + ",catch_time=" + LEX_CAST_STR(p->m_catch_time)
            + ",buy_catch_time=" + LEX_CAST_STR(p->m_buy_catch_time)
            + ",interact_time=" + LEX_CAST_STR(p->m_interact_time)
            + ",rescue_time=" + LEX_CAST_STR(p->m_rescue_time)
            + ",sos_time=" + LEX_CAST_STR(p->m_sos_time)
            + ",be_sos_time_f=" + LEX_CAST_STR(p->m_be_sos_time_f)
            + ",be_sos_time_c=" + LEX_CAST_STR(p->m_be_sos_time_c)
            + ",resist_time=" + LEX_CAST_STR(p->m_resist_time)
            + ",buy_resist_time=" + LEX_CAST_STR(p->m_buy_resist_time)
            + ",interact_cooltime=" + LEX_CAST_STR(p->m_interact_cooltime)
            + ",master_id=" + LEX_CAST_STR(p->m_master_id)
            + ",output=" + LEX_CAST_STR(p->m_output)
            + ",left_time=" + LEX_CAST_STR(p->m_left)
            + ",start_time=" + LEX_CAST_STR(p->m_start_time)
            + ",end_time=" + LEX_CAST_STR(p->m_end_time)
            + ",get_num=" + LEX_CAST_STR(p->m_get_num)
            + " where cid=" + LEX_CAST_STR(cid));
        std::list<int>::iterator it = p->m_servant_list.begin();
        while (it != p->m_servant_list.end())
        {
            InsertSaveDb("update char_servant set master_id=" + LEX_CAST_STR(p->m_cid)
                + " where cid=" + LEX_CAST_STR(*it));
            ++it;
        }
    }
    return -1;
}

int servantMgr::Save_enemy_list(int cid)
{
    //cout << "servantMgr::Save_enemy_list cid=" << cid << endl;
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        InsertSaveDb("delete from char_servant_enemy where cid=" + LEX_CAST_STR(p->m_cid));
        std::list<int>::iterator it = p->m_enemy_list.begin();
        while (it != p->m_enemy_list.end())
        {
            InsertSaveDb("insert into char_servant_enemy set enemyid=" + LEX_CAST_STR(*it)
                + ",cid=" + LEX_CAST_STR(p->m_cid));
            ++it;
        }
    }
    return -1;
}

int servantMgr::Save_loser_list(int cid)
{
    //cout << "servantMgr::Save_loser_list cid=" << cid << endl;
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        InsertSaveDb("delete from char_servant_loser where cid=" + LEX_CAST_STR(p->m_cid));
        std::list<int>::iterator it = p->m_loser_list.begin();
        while (it != p->m_loser_list.end())
        {
            InsertSaveDb("insert into char_servant_loser set loserid=" + LEX_CAST_STR(*it)
                + ",cid=" + LEX_CAST_STR(p->m_cid));
            ++it;
        }
    }
    return -1;
}

int servantMgr::Save_event_list(int cid)
{
    //cout << "servantMgr::Save_event_list cid=" << cid << endl;
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        InsertSaveDb("delete from char_servant_event where cid=" + LEX_CAST_STR(p->m_cid));
        std::list<std::string>::iterator itz = p->m_event_list.begin();
        while (itz != p->m_event_list.end())
        {
            InsertSaveDb("insert into char_servant_event (event,cid) values ('" + GetDb().safestr(*itz)
                + "','" + LEX_CAST_STR(p->m_cid) + "')");
            ++itz;
        }
    }
    return -1;
}

int servantMgr::Save_rescue_list(int cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        InsertSaveDb("delete from char_servant_rescue where cid=" + LEX_CAST_STR(p->m_cid));
        std::list<int>::iterator it = p->m_rescue_list.begin();
        while (it != p->m_rescue_list.end())
        {
            InsertSaveDb("insert into char_servant_rescue (rescue_cid,cid) values ('" + LEX_CAST_STR(*it)
                + "','" + LEX_CAST_STR(p->m_cid) + "')");
            ++it;
        }
    }
    return -1;
}

int servantMgr::Done(int cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        p->done();
        return 0;
    }
    return -1;
}

int servantMgr::ServantListInfo(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (p.get() && pcd.get())
    {
        robj.push_back( Pair("id", cid) );
        robj.push_back( Pair("name", pcd->m_name) );
        robj.push_back( Pair("spic", pcd->m_spic) );
        robj.push_back( Pair("level", pcd->m_level) );
        robj.push_back( Pair("corps", pcd->m_corps_member.get() ? corpsMgr::getInstance()->getCorpsName(pcd->m_corps_member->corps) : "") );
        robj.push_back( Pair("character", p->m_type) );
        robj.push_back( Pair("be_sos_f", iServantBeSOSTime_F - p->m_be_sos_time_f) );
        robj.push_back( Pair("be_sos_c", iServantBeSOSTime_C - p->m_be_sos_time_c) );
        int workTime = p->m_end_time - time(NULL);
        robj.push_back( Pair("workTime", workTime) );
        robj.push_back( Pair("outputJade", p->m_output) );
        robj.push_back( Pair("outputPer", getServantOutput(p->m_cid)) );
        robj.push_back( Pair("interact_get", getInteractJade(pcd->m_level)) );
        robj.push_back( Pair("pressPrice", iServantExploitGold) );
        int fac = workTime / 3600;
        fac += workTime % 3600 > 0 ? 1 : 0;
        if (p->m_type == type_servant)
        {
            json_spirit::Object host;
            boost::shared_ptr<CharData> p_master = GeneralDataMgr::getInstance()->GetCharData(p->m_master_id);
            if (p_master.get())
            {
                host.push_back( Pair("name", p_master->m_name) );
                host.push_back( Pair("level", p_master->m_level) );
                host.push_back( Pair("corps", p_master->m_corps_member.get() ? corpsMgr::getInstance()->getCorpsName(p_master->m_corps_member->corps) : "") );
            }
            robj.push_back( Pair("host", host) );
        }
        robj.push_back( Pair("pumpPrice", (iServantExploitGold * fac)) );
        robj.push_back( Pair("coolTime", (p->m_interact_cooltime - time(NULL))) );
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

bool servantMgr::CheckServant(int master_id, int servant_id)
{
    boost::shared_ptr<charServant> p = GetCharServant(master_id);
    std::list<int>::iterator it = p->m_servant_list.begin();
    while (it != p->m_servant_list.end())
    {
        if (servant_id == *it)
            return true;
        ++it;
    }
    return false;
}

bool servantMgr::CheckRescue(int cid, int rescue_cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    std::list<int>::iterator it = p->m_rescue_list.begin();
    while (it != p->m_rescue_list.end())
    {
        if (rescue_cid == *it)
            return true;
        ++it;
    }
    return false;
}

bool servantMgr::RemoveServant(int master_id, int servant_id)
{
    //cout << "removeServant::master=" << master_id << ",servant_id=" << servant_id << endl;
    boost::shared_ptr<charServant> p = GetCharServant(master_id);
    if (!p.get())
        return false;
    std::list<int>::iterator it = p->m_servant_list.begin();
    while (it != p->m_servant_list.end())
    {
        if (servant_id == *it)
        {
            p->m_servant_list.erase(it);
            return true;
        }
        ++it;
    }
    return false;
}

bool servantMgr::CheckCanList(int master_id, int cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(master_id);
    if (!p.get())
        return false;
    std::list<int>::iterator it = p->m_loser_list.begin();
    while (it != p->m_loser_list.end())
    {
        if (cid == *it)
        {
            return true;
        }
        ++it;
    }
    it = p->m_enemy_list.begin();
    while (it != p->m_enemy_list.end())
    {
        if (cid == *it)
        {
            return true;
        }
        ++it;
    }
    return false;
}

bool servantMgr::RemoveFromCanList(int master_id, int cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(master_id);
    if (!p.get())
        return false;
    bool bHas = false;
    std::list<int>::iterator it = p->m_loser_list.begin();
    while (it != p->m_loser_list.end())
    {
        if (cid == *it)
        {
            p->m_loser_list.erase(it);
            bHas = true;
            Save_loser_list(master_id);
            break;
        }
        ++it;
    }
    it = p->m_enemy_list.begin();
    while (it != p->m_enemy_list.end())
    {
        if (cid == *it)
        {
            p->m_enemy_list.erase(it);
            bHas = true;
            Save_enemy_list(master_id);
            break;
        }
        ++it;
    }
    return bHas;
}

int servantMgr::addServantEvent(int cid, std::string msg)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        if (p->m_event_list.size() >= 10)
        {
            p->m_event_list.pop_front();
        }
        p->m_event_list.push_back(msg);
        return 0;
    }
    return -1;
}

int servantMgr::addServantEnemy(int cid, int eid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        if (p->m_enemy_list.size() >= 20)
        {
            p->m_enemy_list.pop_front();
        }
        p->m_enemy_list.push_back(eid);
        return 0;
    }
    return -1;
}

int servantMgr::addServantLoser(int cid, int lid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        if (p->m_loser_list.size() >= 20)
        {
            p->m_loser_list.pop_front();
        }
        p->m_loser_list.push_back(lid);
        return 0;
    }
    return -1;
}

int servantMgr::getServantType(int cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        return p->m_type;
    }
    return 0;
}

int servantMgr::getServantInfo(CharData& cData, json_spirit::Object& robj)
{
    if (!cData.m_servantOpen)
        return HC_ERROR;
    boost::shared_ptr<charServant> p = GetCharServant(cData.m_id);
    json_spirit::Object info;
    if (p.get())
    {
        info.push_back( Pair("character", p->m_type) );
        info.push_back( Pair("catchNum", iServantCatchTime + p->m_buy_catch_time - p->m_catch_time) );
        info.push_back( Pair("catchPrice", iServantCatchTime_First) );
        info.push_back( Pair("interact", iServantInteractTime - p->m_interact_time) );
        info.push_back( Pair("rescue", iServantRescueTime - p->m_rescue_time) );
        info.push_back( Pair("sos", iServantSOSTime - p->m_sos_time) );
        info.push_back( Pair("be_sos_f", iServantBeSOSTime_F - p->m_be_sos_time_f) );
        info.push_back( Pair("be_sos_c", iServantBeSOSTime_C - p->m_be_sos_time_c) );
        info.push_back( Pair("resist", iServantResistTime + p->m_buy_resist_time - p->m_resist_time) );
        info.push_back( Pair("resistPrice", iServantResistTime_Gold) );
        //劳工产出
        info.push_back( Pair("curJade", p->m_get_num) );
        info.push_back( Pair("totalJade", getMax(cData)) );
        info.push_back( Pair("yushi", LEX_CAST_STR(cData.treasureCount(treasure_type_yushi))) );
        if (p->m_type == type_servant)
        {
            json_spirit::Object host;
            boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(p->m_master_id);
            if (pcd.get())
            {
                host.push_back( Pair("id", pcd->m_id) );
                host.push_back( Pair("name", pcd->m_name) );
                host.push_back( Pair("level", pcd->m_level) );
            }
            info.push_back( Pair("host", host) );
            info.push_back( Pair("coolTime", p->m_interact_cooltime - time(NULL)) );
            info.push_back( Pair("leftTime", p->m_end_time - time(NULL)) );
            info.push_back( Pair("redeemPrice", iServantEscapeGold) );
        }
        else
        {
            int buy_random_times = cData.queryExtraData(char_data_type_daily, char_data_buy_randomservant);
            int needgold = buy_random_times * iServantRandomCatchTime_Gold_Add + iServantRandomCatchTime_Gold_First;
            int random_times = cData.queryExtraData(char_data_type_daily, char_data_randomservant);
            info.push_back( Pair("randomCatchNum", (buy_random_times + iServantRandomCatchTime) - random_times) );
            info.push_back( Pair("randomCatchPrice", needgold) );
        }
    }
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

int servantMgr::getServantEventsList(CharData& cData, json_spirit::Object& robj)
{
    if (!cData.m_servantOpen)
        return HC_ERROR;
    boost::shared_ptr<charServant> p = GetCharServant(cData.m_id);
    json_spirit::Array list;
    if (p.get() && !p->m_event_list.empty())
    {
        std::list<std::string>::iterator it = p->m_event_list.begin();
        while (it != p->m_event_list.end())
        {
            json_spirit::Object info;
            info.push_back( Pair("msg", *it) );
            list.push_back(info);
            ++it;
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int servantMgr::getInteractionList(CharData& cData, json_spirit::Object& robj)
{
    if (!cData.m_servantOpen)
        return HC_ERROR;
    boost::shared_ptr<charServant> p = GetCharServant(cData.m_id);
    if (!p.get())
        return HC_ERROR;
    if (p->m_type == type_free)
        return HC_ERROR;
    json_spirit::Array list;
    std::list<boost::shared_ptr<baseInteract> >::iterator it = m_base_Interact_list.begin();
    while (it != m_base_Interact_list.end())
    {
        if ((*it)->m_type == p->m_type)
        {
            json_spirit::Object info;
            info.push_back( Pair("id", (*it)->m_id) );
            info.push_back( Pair("name", (*it)->m_name) );
            list.push_back(info);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int servantMgr::getServantList(CharData& cData, int type, int purpose, json_spirit::Object& robj)
{
    if (!cData.m_servantOpen)
        return HC_ERROR;
    boost::shared_ptr<charServant> p = GetCharServant(cData.m_id);
    if (!p.get())
        return HC_ERROR;
    json_spirit::Array list;
    switch(type)
    {
        case 1://抓捕家丁
            {
                if (p->m_type == type_servant)
                    return HC_ERROR;
                if (purpose == 1)//手下败将
                {
                    std::list<int>::iterator it = p->m_loser_list.begin();
                    while (it != p->m_loser_list.end())
                    {
                        json_spirit::Object info;
                        if (HC_SUCCESS == ServantListInfo(*it, info))
                            list.push_back(info);
                        ++it;
                    }
                }
                else if (purpose == 2)//夺仆之敌
                {
                    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cData.m_id);
                    my_rl->try_load_enemys();
                    for (std::map<int, my_enemy>::iterator it = my_rl->m_my_enemys.begin();
                            it != my_rl->m_my_enemys.end();
                                ++it)
                    {
                        if (it->second.is_real_enemy())
                        {
                            json_spirit::Object info;
                            if (HC_SUCCESS == ServantListInfo(it->first, info))
                                list.push_back(info);
                        }
                    }
                }
            }
            break;
        case 2://解救家丁
            {
                if (p->m_type == type_servant)
                    return HC_ERROR;
                if (purpose == 1)//军团
                {
                    //军团家丁成员，主人等级差<10
                    if (cData.m_corps_member.get())
                    {
                        splsCorps* cp = corpsMgr::getInstance()->findCorps(cData.m_corps_member->corps);
                        if (cp)
                        {
                            std::list<boost::shared_ptr<corps_member> >::iterator it = cp->_members_list.begin();    //成员
                            while (it != cp->_members_list.end())
                            {
                                if ((*it)->cdata.get() && !CheckServant(cData.m_id,(*it)->cid) && getServantType((*it)->cid) == type_servant && (*it)->cdata->m_servantOpen)
                                {
                                    json_spirit::Object info;
                                    if (HC_SUCCESS == ServantListInfo((*it)->cid, info))
                                        list.push_back(info);
                                }
                                ++it;
                            }
                        }
                    }
                }
                else if (purpose == 2)//好友
                {
                    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cData.m_id);
                    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
                            it != my_rl->m_my_friends.end();
                                ++it)
                    {
                        CharData* pc = it->second->getChar().get();
                        if (pc && pc->m_servantOpen && !CheckServant(cData.m_id, it->first) && getServantType(it->first) == type_servant)
                        {
                            json_spirit::Object info;
                            if (HC_SUCCESS == ServantListInfo(it->first, info))
                                list.push_back(info);
                        }
                        //++it;
                    }
                }
            }
            break;
        case 3://互动
            {
                if (p->m_type == type_master)
                {
                    if (p.get() && !p->m_servant_list.empty())
                    {
                        std::list<int>::iterator it = p->m_servant_list.begin();
                        while (it != p->m_servant_list.end())
                        {
                            json_spirit::Object info;
                            if (HC_SUCCESS == ServantListInfo(*it, info))
                                list.push_back(info);
                            ++it;
                        }
                    }
                }
                else if(p->m_type == type_servant)
                {
                    if (p->m_master_id != 0)
                    {
                        json_spirit::Object info;
                        if (HC_SUCCESS == ServantListInfo(p->m_master_id, info))
                            list.push_back(info);
                    }
                }
            }
            break;
        case 4://剥削
            {
                if (p->m_type != type_master)
                    return HC_ERROR;
                if (p.get() && !p->m_servant_list.empty())
                {
                    std::list<int>::iterator it = p->m_servant_list.begin();
                    while (it != p->m_servant_list.end())
                    {
                        json_spirit::Object info;
                        if (HC_SUCCESS == ServantListInfo(*it, info))
                            list.push_back(info);
                        ++it;
                    }
                }
            }
            break;
        case 5://求救
            {
                if (p->m_type != type_servant)
                    return HC_ERROR;
                if (purpose == 1)//军团
                {
                    if (cData.m_corps_member.get())
                    {
                        splsCorps* cp = corpsMgr::getInstance()->findCorps(cData.m_corps_member->corps);
                        if (cp)
                        {
                            std::list<boost::shared_ptr<corps_member> >::iterator it = cp->_members_list.begin();    //成员
                            while (it != cp->_members_list.end())
                            {
                                if ((*it)->cdata.get() && (*it)->cdata->m_servantOpen && !CheckServant((*it)->cid, cData.m_id) && getServantType((*it)->cid) != type_servant)
                                {
                                    json_spirit::Object info;
                                    if (HC_SUCCESS == ServantListInfo((*it)->cid, info))
                                        list.push_back(info);
                                }
                                ++it;
                            }
                        }
                    }
                }
                else if (purpose == 2)//好友
                {
                    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cData.m_id);
                    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_friends.begin();
                            it != my_rl->m_my_friends.end();
                                ++it)
                    {
                        CharData* pc = it->second->getChar().get();
                        if (pc && pc->m_servantOpen && !CheckServant(it->first, cData.m_id) && getServantType(it->first) != type_servant)
                        {
                            json_spirit::Object info;
                            if (HC_SUCCESS == ServantListInfo(it->first, info))
                                list.push_back(info);
                        }
                        //++it;
                    }
                }
            }
            break;
        default:
            return HC_ERROR;
            break;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int servantMgr::getMax(CharData& cData)
{
    int raceFac = 0;
    boost::shared_ptr<CharRaceData> rd = RaceMgr::getInstance()->getRaceData(cData.m_id);
    if (!rd.get() || !rd->getChar())
    {
        raceFac = 100;
    }
    else
    {
        if (rd->m_rank >= 100 || rd->m_rank <= 0)
        {
            raceFac = 100;
        }
        else if(rd->m_rank >= 60)
        {
            raceFac = 60;
        }
        else
        {
            raceFac = rd->m_rank;
        }
    }
    int max_yushi = cData.m_level * iServantMaxFac - raceFac * cData.m_level * 2 + 3000;
    //家丁收益系数
    servantRealReward(max_yushi);
    return max_yushi;
}

int servantMgr::DealServantAction(CharData& cData, int type, int extra_type, int id, int aid, json_spirit::Object& robj)
{
    if (!cData.m_servantOpen)
        return HC_ERROR;
    boost::shared_ptr<charServant> p = GetCharServant(cData.m_id);
    boost::shared_ptr<charServant> pd = GetCharServant(id);
    if (!p.get()||!pd.get())
        return HC_ERROR;
    switch(type)
    {
        case 1://抓捕
            {
                if (p->m_type == type_servant || pd->m_type == type_master)
                {
                    return HC_ERROR;
                }
                if (p->m_catch_time >= iServantCatchTime + p->m_buy_catch_time)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                if (p->m_servant_list.size() >= 3)
                {
                    return HC_ERROR_SERVANT_TOO_MORE;
                }
                if (pd->m_interact_cooltime >= time(NULL))
                {
                    return HC_ERROR_SERVANT_CD;
                }
                if (CheckServant(p->m_cid, pd->m_cid))
                {
                    return HC_ERROR;
                }

                int ret = HC_SUCCESS;
                Combat* pCombat = NULL;
                if (pd->m_type == type_servant)
                {
                    pd = GetCharServant(pd->m_master_id);
                    if (!pd.get())
                    {
                        return HC_ERROR;
                    }
                    //是否正在家丁战斗中
                    if (p->m_fight_with > 0 || pd->m_fight_with > 0)
                    {
                        return HC_ERROR_TARGET_IS_BUSY;
                    }
                    pCombat = createServantCombat(cData.m_id, pd->m_cid, type, id, ret);//creatservantcombat
                    //cout << "catch a servant!!!a rob fight" << endl;
                }
                else
                {
                    //是否正在家丁战斗中
                    if (p->m_fight_with > 0 || pd->m_fight_with > 0)
                    {
                        return HC_ERROR_TARGET_IS_BUSY;
                    }
                    pCombat = createServantCombat(cData.m_id, id, type, 0, ret);//creatservantcombat
                }
                if (HC_SUCCESS == ret && pCombat)
                {
                    //设置主人为战斗
                    pd->m_fight_with = p->m_cid;
                    p->m_fight_with = pd->m_cid;

                    ++p->m_catch_time;
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "attackServant") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                    std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                    if (account.get())
                    {
                        account->Send(msg);
                    }
                    InsertCombat(pCombat);
                    Save_data(cData.m_id);
                    //日常任务
                    dailyTaskMgr::getInstance()->updateDailyTask(cData,daily_task_servant_catch);
                    //更新按钮次数
                    int buy_times = cData.queryExtraData(char_data_type_daily, char_data_buy_race);
                    int leftTimes = iServantCatchTime + p->m_buy_catch_time - p->m_catch_time;
                    cData.notifyEventState(top_level_event_servant, 0, leftTimes);
                }
                return ret;
            }
            break;
        case 2://抢夺
            {
                if (p->m_type == type_servant || pd->m_type != type_master)
                {
                    return HC_ERROR;
                }
                if (p->m_catch_time >= iServantCatchTime + p->m_buy_catch_time)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                if (p->m_servant_list.size() >= 3)
                {
                    return HC_ERROR_SERVANT_TOO_MORE;
                }
                if (CheckServant(p->m_cid, pd->m_cid))
                {
                    return HC_ERROR;
                }
                //是否正在家丁战斗中
                if (p->m_fight_with > 0 || pd->m_fight_with > 0)
                {
                    return HC_ERROR_TARGET_IS_BUSY;
                }

                int ret = HC_SUCCESS;
                Combat* pCombat = createServantCombat(cData.m_id, id, type, 0, ret);//creatservantcombat
                if (HC_SUCCESS == ret && pCombat)
                {
                    //设置主人为战斗
                    pd->m_fight_with = cData.m_id;
                    p->m_fight_with = id;
                    
                    ++p->m_catch_time;
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "attackServant") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                    std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                    if (account.get())
                    {
                        account->Send(msg);
                    }
                    InsertCombat(pCombat);
                    Save_data(cData.m_id);
                    //日常任务
                    dailyTaskMgr::getInstance()->updateDailyTask(cData,daily_task_servant_catch);
                    //更新按钮次数
                    int buy_times = cData.queryExtraData(char_data_type_daily, char_data_buy_race);
                    int leftTimes = iServantCatchTime + p->m_buy_catch_time - p->m_catch_time;
                    cData.notifyEventState(top_level_event_servant, 0, leftTimes);
                }
                return ret;
            }
            break;
        case 3://解救
            {
                // 1家丁不能解救别人 2只能解救家丁
                if (p->m_type == type_servant || pd->m_type != type_servant)
                {
                    return HC_ERROR;
                }
                if (p->m_rescue_time >= iServantRescueTime)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                if (pd->m_interact_cooltime >= time(NULL))
                {
                    return HC_ERROR_SERVANT_CD;
                }
                if (CheckRescue(cData.m_id,id))
                {
                    return HC_ERROR_SERVANT_RESCUE;
                }
                if (extra_type == 1)//解救军团成员
                {
                    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                    if (!cData.m_corps_member.get() || !pcd.get() || !pcd->m_corps_member.get())
                    {
                        return HC_ERROR;
                    }
                    if (cData.m_corps_member->corps != pcd->m_corps_member->corps)
                    {
                        return HC_ERROR;
                    }
                }
                else if(extra_type == 2)//解救好友
                {
                    if (!cData.check_friend(id))
                    {
                        return HC_ERROR;
                    }
                }
                boost::shared_ptr<CharData> pcd_master = GeneralDataMgr::getInstance()->GetCharData(pd->m_master_id);
                if (!pcd_master.get() || cData.m_level - pcd_master->m_level >= 10 || pcd_master->m_level - cData.m_level >= 10)
                {
                    return HC_ERROR_SERVANT_LEVEL;
                }
                boost::shared_ptr<charServant> pdd = GetCharServant(pd->m_master_id);
                if (!pdd.get())
                {
                    return HC_ERROR;
                }
                //解救对象的主人正在家丁战斗中，不能解救，因为可能解救对象的主人发生变化
                if (pdd->m_fight_with > 0
                    || p->m_fight_with > 0)
                {
                    return HC_ERROR_TARGET_IS_BUSY;
                }
                int ret = HC_SUCCESS;
                Combat* pCombat = createServantCombat(cData.m_id, pd->m_master_id, type, id, ret);//creatservantcombat
                if (HC_SUCCESS == ret && pCombat)
                {
                    //设置主人为战斗
                    pdd->m_fight_with = id;
                    p->m_fight_with = pd->m_master_id;
                    
                    ++p->m_rescue_time;
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "attackServant") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                    std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                    if (account.get())
                    {
                        account->Send(msg);
                    }
                    InsertCombat(pCombat);
                    Save_data(cData.m_id);
                    p->m_rescue_list.push_back(id);
                    Save_rescue_list(cData.m_id);
                    //日常任务
                    dailyTaskMgr::getInstance()->updateDailyTask(cData,daily_task_servant_rescue);
                }
                return ret;
            }
            break;
        case 4://互动
            {
                if ((p->m_type == type_servant && p->m_interact_cooltime >= time(NULL))
                    || (p->m_type == type_master && pd->m_interact_cooltime >= time(NULL)))
                {
                    return HC_ERROR_SERVANT_CD;
                }
                if (p->m_interact_time >= iServantInteractTime)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                boost::shared_ptr<CharData> pcd_to = GeneralDataMgr::getInstance()->GetCharData(cData.m_id);
                if (!pcd.get() || !pcd_to.get())
                {
                    return HC_ERROR;
                }
                if (p->m_type == type_servant && CheckServant(pd->m_cid, p->m_cid))
                {
                    boost::shared_ptr<baseInteract> pbi = getInteraction(aid);
                    if (pbi.get())
                    {
                        ++p->m_interact_time;
                        p->m_interact_cooltime = time(NULL) + iServantCD;
                        std::string msg = pbi->m_memo;
                        str_replace(msg, "$N", pcd->m_name);
                        str_replace(msg, "$N", pcd->m_name,true);
                        addServantEvent(cData.m_id, msg);
                        msg = pbi->m_memo2;
                        str_replace(msg, "$N", pcd_to->m_name);
                        str_replace(msg, "$N", pcd_to->m_name,true);
                        addServantEvent(id, msg);
                        //加玉石
                        int num = getInteractJade(pcd->m_level);
                        
                        cData.addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                        gold_cost_tencent(&cData,0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                        add_statistics_of_treasure_cost(cData.m_id, cData.m_ip_address, treasure_type_yushi, num, treasure_servant, 1, cData.m_union_id, cData.m_server_id);
                        msg = strServantGetMsg;
                        str_replace(msg, "$N", LEX_CAST_STR(num));
                        robj.push_back( Pair("msg", msg) );
                        robj.push_back( Pair("num", num) );
                        Save_data(cData.m_id);
                        Save_event_list(cData.m_id);
                        Save_event_list(id);

                        cData.m_open_info_change = true;
                    }
                    else
                    {
                        return HC_ERROR;
                    }
                }
                else if(p->m_type == type_master && CheckServant(p->m_cid, pd->m_cid))
                {
                    boost::shared_ptr<baseInteract> pbi = getInteraction(aid);
                    if (pbi.get())
                    {
                        ++p->m_interact_time;
                        pd->m_interact_cooltime = time(NULL) + iServantCD;
                        std::string msg = pbi->m_memo;
                        str_replace(msg, "$N", pcd->m_name);
                        str_replace(msg, "$N", pcd->m_name,true);
                        addServantEvent(cData.m_id, msg);
                        msg = pbi->m_memo2;
                        str_replace(msg, "$N", pcd_to->m_name);
                        str_replace(msg, "$N", pcd_to->m_name,true);
                        addServantEvent(id, msg);
                        //加玉石
                        int num = getInteractJade(pcd->m_level);
                        cData.addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                        gold_cost_tencent(&cData,0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                        add_statistics_of_treasure_cost(cData.m_id, cData.m_ip_address, treasure_type_yushi, num, treasure_servant, 1, cData.m_union_id, cData.m_server_id);
                        msg = strServantGetMsg;
                        str_replace(msg, "$N", LEX_CAST_STR(num));
                        robj.push_back( Pair("msg", msg) );
                        robj.push_back( Pair("num", num) );
                        Save_data(cData.m_id);
                        Save_event_list(cData.m_id);
                        Save_event_list(id);

                        cData.m_open_info_change = true;
                    }
                    else
                    {
                        return HC_ERROR;
                    }
                }
                //日常任务
                dailyTaskMgr::getInstance()->updateDailyTask(cData,daily_task_servant);
            }
            break;
        case 5://提取
            {
                if (CheckServant(cData.m_id, id))
                {
                    //加玉石
                    int num = pd->m_output;
                    if ((p->m_get_num + num) >= getMax(cData))
                    {
                        num = getMax(cData) - p->m_get_num;
                    }
                    if(num <= 0)
                    {
                        return HC_ERROR_SERVANT_YUSHIMAX;
                    }
                    cData.addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                    gold_cost_tencent(&cData,0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                    add_statistics_of_treasure_cost(cData.m_id, cData.m_ip_address, treasure_type_yushi, num, treasure_servant, 1, cData.m_union_id, cData.m_server_id);
                    std::string msg = strServantGetMsg;
                    str_replace(msg, "$N", LEX_CAST_STR(num));
                    robj.push_back( Pair("msg", msg) );
                    robj.push_back( Pair("num", num) );
                    p->m_get_num += num;
                    pd->m_output = 0;
                    Save_data(cData.m_id);
                    Save_data(pd->m_cid);
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 6://剥削
            {
                if (CheckServant(cData.m_id, id))
                {
                    //正在被抢夺的时候，不能剥削
                    if (p->m_fight_with > 0)
                    {
                        return HC_ERROR_TARGET_IS_BUSY;
                    }
                    if (-1 == cData.addGold(-iServantExploitGold))
                    {
                        return HC_ERROR_NOT_ENOUGH_GOLD;
                    }
                    add_statistics_of_gold_cost(cData.m_id,cData.m_ip_address,iServantExploitGold,gold_cost_for_servant_exploit, cData.m_union_id, cData.m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(&cData,iServantExploitGold,gold_cost_for_servant_exploit);
#endif
                    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                    if (!pcd.get())
                    {
                        return HC_ERROR;
                    }
                    //加玉石
                    int num = getServantOutput(pd->m_cid);
                    if (pd->m_end_time - time(NULL) >= 3600 && pd->m_left >= 6)
                    {
                        pd->m_end_time -= 3600;
                        pd->m_left -= 6;
                    }
                    else
                    {
                        pd->m_end_time = time(NULL);
                        num = pd->m_left * getServantOutput(pd->m_cid) / 6;
                        pd->m_left = 0;
                    }
                    if (pd->m_left == 0)
                        pd->done();
                    pd->m_output += num;
                    Save_data(cData.m_id);
                    Save_data(pd->m_cid);
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 7://抽干
            {
                if (CheckServant(cData.m_id, id))
                {
                    //正在被抢夺的时候，不能抽干
                    if (p->m_fight_with > 0)
                    {
                        return HC_ERROR_TARGET_IS_BUSY;
                    }
                    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                    if (!pcd.get())
                    {
                        return HC_ERROR;
                    }
                    if(getMax(cData) - p->m_get_num <= 0)
                    {
                        return HC_ERROR_SERVANT_YUSHIMAX;
                    }
                    int left_sec = pd->m_end_time - time(NULL);
                    if (left_sec > 0)
                    {
                        int fac = left_sec / 3600;
                        fac += left_sec % 3600 > 0 ? 1 : 0;
        
                        if (-1 == cData.addGold(-(iServantExploitGold * fac)))
                        {
                            return HC_ERROR_NOT_ENOUGH_GOLD;
                        }
                        add_statistics_of_gold_cost(cData.m_id,cData.m_ip_address,iServantExploitGold * fac,gold_cost_for_servant_exploit2, cData.m_union_id, cData.m_server_id);
#ifdef QQ_PLAT
                        gold_cost_tencent(&cData,iServantExploitGold * fac,gold_cost_for_servant_exploit2);
#endif
                    }
                    if (pd->m_left < 0)
                    {
                        pd->m_left = 0;
                    }
                    //加玉石
                    int add_yushi = pd->m_left * pcd->m_level/2;
                    servantRealReward(add_yushi);
                    pd->m_output += add_yushi;
                    
                    pd->m_left = 0;
                    int num = pd->done();
                    if (num != 0)
                    {
                        std::string msg = strServantGetMsg;
                        str_replace(msg, "$N", LEX_CAST_STR(num));
                        robj.push_back( Pair("msg", msg) );
                        robj.push_back( Pair("num", num) );
                    }
                    Save_data(cData.m_id);
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 8://释放
            {
                if (pd->m_master_id == cData.m_id)
                {
                    if (pd->m_interact_cooltime >= time(NULL))
                    {
                        return HC_ERROR_SERVANT_CD;
                    }
                    //如果主人正在家丁战斗，不能赎身，因为主人可能会变化
                    if (p->m_fight_with > 0)
                    {
                        return HC_ERROR_TARGET_IS_BUSY;
                    }
                    int num = pd->m_output * 80 / 100;
                    if ((p->m_get_num + num) >= getMax(cData))
                    {
                        num = getMax(cData) - p->m_get_num;
                    }
                    if (num > 0)
                    {
                        cData.addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                        gold_cost_tencent(&cData,0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                        add_statistics_of_treasure_cost(cData.m_id, cData.m_ip_address, treasure_type_yushi, num, treasure_servant, 1, cData.m_union_id, cData.m_server_id);
                        p->m_get_num += num;
                    }

                    pd->stop();
                    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                    if (!pcd.get())
                        return HC_ERROR;
                    std::string msg = strServantReleaseSucAtk;
                    str_replace(msg, "$N", pcd->m_name);
                    str_replace(msg, "$N", pcd->m_name,true);
                    addServantEvent(cData.m_id, msg);
                    msg = strServantReleaseSucDef;
                    str_replace(msg, "$N", cData.m_name);
                    str_replace(msg, "$N", cData.m_name,true);
                    addServantEvent(id, msg);
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 9://赎身
            {
                if (p->m_master_id == id && cData.m_vip >= iServantEscapeVIP)
                {
                    if (p->m_type == type_servant && p->m_interact_cooltime >= time(NULL))
                    {
                        return HC_ERROR_SERVANT_CD;
                    }
                    //如果主人正在家丁战斗，不能赎身，因为主人可能会变化
                    if (pd->m_fight_with > 0)
                    {
                        return HC_ERROR_TARGET_IS_BUSY;
                    }
                    if (-1 == cData.addGold(-iServantEscapeGold))
                    {                        
                        return HC_ERROR_NOT_ENOUGH_GOLD;
                    }
                    add_statistics_of_gold_cost(cData.m_id,cData.m_ip_address,iServantEscapeGold,gold_cost_for_servant_escape, cData.m_union_id, cData.m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(&cData,iServantEscapeGold,gold_cost_for_servant_escape);
#endif
                    int num = pd->m_output * 80 / 100;
                    if ((p->m_get_num + num) >= getMax(cData))
                    {
                        num = getMax(cData) - p->m_get_num;
                    }
                    if (num > 0)
                    {
                        cData.addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                        gold_cost_tencent(&cData,0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                        add_statistics_of_treasure_cost(cData.m_id, cData.m_ip_address, treasure_type_yushi, num, treasure_servant, 1, cData.m_union_id, cData.m_server_id);
                        p->m_get_num += num;
                    }
                    p->stop();

                    std::string msg = strServantRedeemSucAtk;
                    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                    if (pcd.get())
                    {
                        str_replace(msg, "$N", pcd->m_name);
                        str_replace(msg, "$N", pcd->m_name,true);
                        addServantEvent(cData.m_id, msg);
                    }
                    msg = strServantRedeemSucDef;
                    str_replace(msg, "$N", cData.m_name);
                    str_replace(msg, "$N", cData.m_name,true);
                    str_replace(msg, "$n", cData.m_name);
                    str_replace(msg, "$n", cData.m_name,true);
                    addServantEvent(id, msg);

                    Save_data(cData.m_id);
                    Save_event_list(cData.m_id);
                    Save_event_list(cData.m_id);
                }
                else
                {
                    return HC_ERROR;
                }
            }
            break;
        case 10://反抗
            {
                if (p->m_type != type_servant)
                {
                    return HC_ERROR;
                }
                //无法在互动冷却中反抗
                if (p->m_interact_cooltime >= time(NULL))
                {
                    return HC_ERROR_SERVANT_CD;
                }
                if (p->m_resist_time >= iServantResistTime + p->m_buy_resist_time)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }

                boost::shared_ptr<charServant> pdd = GetCharServant(p->m_master_id);
                if (!pdd.get())
                {
                    return HC_ERROR;
                }
                //主人是否正在家丁战斗中
                if (pdd->m_fight_with > 0)
                {
                    return HC_ERROR_TARGET_IS_BUSY;
                }
                int ret = HC_SUCCESS;
                Combat* pCombat = createServantCombat(cData.m_id, p->m_master_id, type, 0, ret);//creatservantcombat
                if (HC_SUCCESS == ret && pCombat)
                {
                    //设置主人为战斗
                    pdd->m_fight_with = cData.m_id;

                    ++p->m_resist_time;
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "attackServant") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                    std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                    if (account.get())
                    {
                        account->Send(msg);
                    }
                    InsertCombat(pCombat);
                    Save_data(cData.m_id);
                }
                return ret;
            }
            break;
        case 11://求救
            {
                //家丁才能发起求救，并且求救对象不能是家丁
                if (p->m_type != type_servant || pd->m_type == type_servant)
                {
                    return HC_ERROR;
                }
                //不能向主人求救
                if (p->m_master_id == id)
                {
                    return HC_ERROR;
                }
                //无法在互动冷却中求救
                if (p->m_interact_cooltime >= time(NULL))
                {
                    return HC_ERROR_SERVANT_CD;
                }
                //求救次数
                if (p->m_sos_time >= iServantSOSTime)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                if (extra_type == 1)//求救军团成员
                {
                    if (pd->m_be_sos_time_c >= iServantBeSOSTime_C)
                    {
                        return HC_ERROR_NOT_ENOUGH_TIME;
                    }
                    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(id);
                    if (!cData.m_corps_member.get() || !pcd.get() || !pcd->m_corps_member.get())
                    {
                        return HC_ERROR;
                    }
                    if (cData.m_corps_member->corps != pcd->m_corps_member->corps)
                    {
                        return HC_ERROR;
                    }
                }
                else if(extra_type == 2)//求救好友
                {
                    if (pd->m_be_sos_time_f >= iServantBeSOSTime_F)
                    {
                        return HC_ERROR_NOT_ENOUGH_TIME;
                    }
                    if (!cData.check_friend(id))
                    {
                        return HC_ERROR;
                    }
                }

                boost::shared_ptr<charServant> pdd = GetCharServant(p->m_master_id);
                if (!pdd.get())
                {
                    return HC_ERROR;
                }
                //主人是否正在家丁战斗中
                if (pdd->m_fight_with > 0 || pd->m_fight_with > 0)
                {
                    return HC_ERROR_TARGET_IS_BUSY;
                }

                int ret = HC_SUCCESS;
                Combat* pCombat = createServantCombat(id, p->m_master_id, type, cData.m_id, ret);//creatservantcombat
                if (HC_SUCCESS == ret && pCombat)
                {
                    pCombat->set_extra_viewer(cData.m_name);
                    //设置主人为战斗
                    pdd->m_fight_with = id;
                    pd->m_fight_with = p->m_master_id;
                    ++p->m_sos_time;
                    if (extra_type == 1)//求救军团成员
                    {
                        ++pd->m_be_sos_time_c;
                    }
                    else if(extra_type == 2)//求救好友
                    {
                        ++pd->m_be_sos_time_f;
                    }
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "attackServant") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                    std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cData.m_name);
                    if (account.get())
                    {
                        account->Send(msg);
                    }
                    InsertCombat(pCombat);
                    Save_data(cData.m_id);
                }
                return ret;
            }
            break;
        case 12://购买抓捕次数
            {
                int cost = iServantCatchTime_First;
                if (cost < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                if (-1 == cData.addGold(-cost))
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                add_statistics_of_gold_cost(cData.m_id,cData.m_ip_address,cost,gold_cost_for_servant_buy, cData.m_union_id, cData.m_server_id);
#ifdef QQ_PLAT
                gold_cost_tencent(&cData,cost,gold_cost_for_servant_buy);
#endif
                ++p->m_buy_catch_time;
                Save_data(cData.m_id);
            }
            break;
        case 13://购买反抗次数
            {
                #if 0
                if (-1 == cData.addGold(-iServantResistTime_Gold))
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                add_statistics_of_gold_cost(cData.m_id,cData.m_ip_address,iServantResistTime_Gold,gold_cost_for_servant);
                ++p->m_buy_resist_time;
                Save_data(cData.m_id);
                #endif
            }
            break;
        case 14://购买随机抓捕次数
            {
                int buy_random_times = cData.queryExtraData(char_data_type_daily, char_data_buy_randomservant);
                int needgold = buy_random_times * iServantRandomCatchTime_Gold_Add + iServantRandomCatchTime_Gold_First;
                if (needgold < 0)
                    needgold = 0;
                if (needgold)
                {
                    if (-1 == cData.addGold(-needgold))
                        return HC_ERROR_NOT_ENOUGH_GOLD;
                    add_statistics_of_gold_cost(cData.m_id,cData.m_ip_address,needgold,gold_cost_for_servant_buy_random, cData.m_union_id, cData.m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(&cData,needgold,gold_cost_for_servant_buy_random);
#endif
                }
                cData.setExtraData(char_data_type_daily, char_data_buy_randomservant, buy_random_times + 1);
            }
            break;
        case 15://随机抓捕
            {
                int random_times = cData.queryExtraData(char_data_type_daily, char_data_randomservant);
                int buy_random_times = cData.queryExtraData(char_data_type_daily, char_data_buy_randomservant);
                if (random_times >= buy_random_times + iServantRandomCatchTime)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                //家丁不能随机抓捕别人
                if (p->m_type == type_servant)
                {
                    return HC_ERROR;
                }
                //有三个家丁也不能抓了
                if (p->m_servant_list.size() >= 3)
                {
                    return HC_ERROR_SERVANT_TOO_MORE;
                }
                if (p->m_fight_with > 0)
                {
                    return HC_ERROR_TARGET_IS_BUSY;
                }
                //抓捕11-100随机玩家
                std::vector<int> list;
                list.clear();
                //cout << "list_size=" << list.size();
                RaceMgr::getInstance()->getRandomServantList(cData.m_id,cData.m_area,list);
                while (!list.empty())
                {
                    std::vector<int>::iterator it = my_random(0, list.size() - 1) + list.begin();
                    //cout << "list_size=" << list.size() << ",pos=" << pos << endl;
                    boost::shared_ptr<charServant> ptmp = GetCharServant(*it);
                    if (!ptmp.get())
                    {
                        list.erase(it);
                        continue;
                    }
                    //抓捕对象是主人
                    if (ptmp->m_type == type_master)
                    {
                        //抓捕对象正在战斗中
                        if (ptmp->m_fight_with > 0)
                        {
                            list.erase(it);
                            continue;
                        }
                        int ret = HC_SUCCESS;
                        Combat* pCombat = createServantCombat(cData.m_id, ptmp->m_cid, 2, 0, ret);//creatservantcombat
                        if (HC_SUCCESS == ret && pCombat)
                        {
                            cData.setExtraData(char_data_type_daily, char_data_randomservant, random_times + 1);
                            json_spirit::Object obj;
                            obj.push_back( Pair("cmd", "attackServant") );
                            obj.push_back( Pair("s", 200) );
                            obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                            std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                            if (account.get())
                            {
                                account->Send(msg);
                            }
                            InsertCombat(pCombat);
                            Save_data(cData.m_id);
                            //日常任务
                            dailyTaskMgr::getInstance()->updateDailyTask(cData,daily_task_servant_catch);
                            return ret;
                        }
                        else
                        {
                            list.erase(it);
                            continue;
                        }
                    }
                    else
                    {
                        int ret = HC_SUCCESS;
                        Combat* pCombat = NULL;
                        if (ptmp->m_type == type_servant)
                        {
                            //家丁有冷却不能抓，是自己的家丁不能抓
                            if (ptmp->m_interact_cooltime >= time(NULL)
                                || CheckServant(p->m_cid, ptmp->m_cid))
                            {
                                list.erase(it);
                                continue;
                            }
                            //家丁的主人在家丁战斗中不能抓
                            boost::shared_ptr<charServant> pdd = GetCharServant(ptmp->m_master_id);
                            if (!pdd.get()|| pdd->m_fight_with > 0)
                            {
                                list.erase(it);
                                continue;
                            }
                            pCombat = createServantCombat(cData.m_id, ptmp->m_master_id, 1, ptmp->m_cid, ret);//creatservantcombat
                        }
                        else
                        {
                            //自由身在家丁战斗中不能抓
                            if (ptmp->m_fight_with > 0)
                            {
                                list.erase(it);
                                continue;
                            }
                            pCombat = createServantCombat(cData.m_id, ptmp->m_cid, 1, 0, ret);//creatservantcombat
                        }
                        if (HC_SUCCESS == ret && pCombat)
                        {
                            cData.setExtraData(char_data_type_daily, char_data_randomservant, random_times + 1);
                            json_spirit::Object obj;
                            obj.push_back( Pair("cmd", "attackServant") );
                            obj.push_back( Pair("s", 200) );
                            obj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
                            std::string msg = json_spirit::write(obj, json_spirit::raw_utf8);
                            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
                            if (account.get())
                            {
                                account->Send(msg);
                            }
                            InsertCombat(pCombat);
                            Save_data(cData.m_id);
                            //日常任务
                            dailyTaskMgr::getInstance()->updateDailyTask(cData,daily_task_servant_catch);
                            return HC_SUCCESS;
                        }
                        else
                        {
                            list.erase(it);
                            continue;
                        }
                    }
                }
                return HC_ERROR_SERVANT;
            }
            break;
        default:
            break;
    }
    return HC_SUCCESS;
}

boost::shared_ptr<baseInteract> servantMgr::getInteraction(int id)
{
    std::list<boost::shared_ptr<baseInteract> >::iterator it = m_base_Interact_list.begin();
    while (it != m_base_Interact_list.end())
    {
        if ((*it)->m_id == id)
        {
            return *it;
        }
        ++it;
    }
    boost::shared_ptr<baseInteract> pbi;
    pbi.reset();
    return pbi;
}

boost::shared_ptr<charServant> servantMgr::GetCharServant(int cid)
{
    std::map<uint64_t, boost::shared_ptr<charServant> >::iterator it = m_charServant_list.find(cid);
    if (it != m_charServant_list.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (pcd.get() && pcd->m_servantOpen)
            return open(cid);
        else
        {
            boost::shared_ptr<charServant> pcs;
            pcs.reset();
            return pcs;
        }
    }
}

boost::shared_ptr<charServant> servantMgr::open(int cid)
{
    std::map<uint64_t, boost::shared_ptr<charServant> >::iterator it = m_charServant_list.find(cid);
    if (it == m_charServant_list.end())
    {
        boost::shared_ptr<charServant> pcs;
        pcs.reset(new charServant);
        pcs->m_cid = cid;
        m_charServant_list[cid] = pcs;
        InsertSaveDb("insert into char_servant set cid=" + LEX_CAST_STR(cid));
        //随机推送3个玩家到手下败将
        int cnt = 0;
        std::vector<int> list;
        list.clear();
        GeneralDataMgr::getInstance()->getRandomServantList(list);
        cout << "list.size=" << list.size() <<endl;
        while (!list.empty() && cnt < 3)
        {
            std::vector<int>::iterator it = my_random(0, list.size() - 1) + list.begin();
            if (*it == cid)
            {
                list.erase(it);
                continue;
            }
            boost::shared_ptr<charServant> ptmp = GetCharServant(*it);
            if (!ptmp.get())
            {
                list.erase(it);
                continue;
            }
            addServantLoser(cid,*it);
            ++cnt;
            list.erase(it);
        }
        return pcs;
    }
    else
    {
        return it->second;
    }
}

void servantMgr::deleteChar(int cid)
{
    boost::shared_ptr<charServant> p = GetCharServant(cid);
    if (p.get())
    {
        if (!p->m_servant_list.empty())
        {
            std::list<int>::iterator it = p->m_servant_list.begin();
            while (it != p->m_servant_list.end())
            {
                boost::shared_ptr<charServant> p_servant = GetCharServant(*it);
                if (p_servant.get() && p_servant->m_type == type_servant)
                {
                    p_servant->stop();
                    it = p->m_servant_list.begin();
                }
                else
                {
                    ++it;
                }
            }
        }
        p->stop();
    }
    InsertSaveDb("delete from char_servant where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_servant_enemy where cid=" + LEX_CAST_STR(cid) + " or enemyid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_servant_loser where cid=" + LEX_CAST_STR(cid) + " or loserid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_servant_event where cid=" + LEX_CAST_STR(cid));
}

void servantMgr::resetAll()
{
    std::map<uint64_t, boost::shared_ptr<charServant> >::iterator it = m_charServant_list.begin();
    while (it != m_charServant_list.end())
    {
        it->second->m_catch_time = 0;
        it->second->m_buy_catch_time = 0;
        it->second->m_interact_time = 0;
        it->second->m_rescue_time = 0;
        it->second->m_sos_time = 0;
        it->second->m_be_sos_time_f = 0;
        it->second->m_be_sos_time_c = 0;
        it->second->m_resist_time = 0;
        it->second->m_buy_resist_time = 0;
        it->second->m_get_num = 0;
        it->second->m_interact_cooltime = 0;
        it->second->m_rescue_list.clear();
        Save_rescue_list(it->first);
        Save_data(it->first);
        ++it;
    }
}

int servantMgr::combatResult(Combat* pCombat)    //战斗结束
{
    boost::shared_ptr<charServant> pcs_atk = GetCharServant(pCombat->m_attacker->getCharId());
    boost::shared_ptr<charServant> pcs_def = GetCharServant(pCombat->m_defender->getCharId());
    if (!pcs_atk.get() || !pcs_def.get())
    {
        return HC_ERROR;
    }
    if (pCombat->m_state == attacker_win)
    {
        boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
        boost::shared_ptr<CharData> pcd_def = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_defender->getCharId());
        if (!pcd_atk.get() || !pcd_def.get())
        {
            return HC_ERROR;
        }
        //战斗双方空闲了
        pcs_def->m_fight_with = 0;
        pcs_atk->m_fight_with = 0;
        switch(pCombat->m_extra_data[0])
        {
            case 1://抓捕
                {
                    //抓捕别人的家丁
                    if (pCombat->m_extra_data[1] != 0)
                    {
                        boost::shared_ptr<charServant> pcs_change = GetCharServant(pCombat->m_extra_data[1]);
                        if (pcs_change.get())
                        {
                            int num = pcs_change->m_output * 80 / 100;
                            if ((pcs_def->m_get_num + num) >= getMax(*pcd_def))
                            {
                                num = getMax(*pcd_def) - pcs_def->m_get_num;
                            }

                            if (num > 0)
                            {
                                pcd_def->addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                                gold_cost_tencent(pcd_def.get(),0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                                add_statistics_of_treasure_cost(pcd_def->m_id, pcd_def->m_ip_address, treasure_type_yushi, num, treasure_servant, 1, pcd_def->m_union_id,pcd_def->m_server_id);
                                pcs_def->m_get_num += num;
                            }
                            pcs_change->stop();
                            pcs_change->m_master_id = pcs_atk->m_cid;
                            pcs_change->m_type = type_servant;
                            pcs_change->m_start_time = time(NULL);
                            pcs_change->m_end_time = pcs_change->m_start_time + 43200;
                            pcs_change->m_left = 72;
                            pcs_change->m_output = getServantOutput(pcs_change->m_cid)/6;
                            pcs_change->start();
                            //从可抓捕列表移除
                            RemoveFromCanList(pcs_atk->m_cid,pcs_change->m_cid);
                            pcs_atk->m_servant_list.push_back(pCombat->m_extra_data[1]);
                            pcs_atk->m_type = type_master;
                            boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(pcs_change->m_cid);
                            if (!pcd.get())
                            {
                                return HC_ERROR;
                            }
                            std::string msg = strServantRobSucAtk;
                            str_replace(msg, "$N", pcd_def->m_name);
                            str_replace(msg, "$N", pcd_def->m_name,true);
                            str_replace(msg, "$n", pcd->m_name);
                            str_replace(msg, "$n", pcd->m_name,true);
                            addServantEvent(pcs_atk->m_cid, msg);
                            msg = strServantRobSucDef;
                            str_replace(msg, "$N", pcd_atk->m_name);
                            str_replace(msg, "$N", pcd_atk->m_name,true);
                            str_replace(msg, "$n", pcd->m_name);
                            str_replace(msg, "$n", pcd->m_name,true);
                            addServantEvent(pcs_def->m_cid, msg);
                            if (!CheckCanList(pcs_def->m_cid,pcs_atk->m_cid))
                                addServantEnemy(pcs_def->m_cid,pcs_atk->m_cid);

                            //抓捕仇敌成功
                            if (Singleton<relationMgr>::Instance().is_my_enemy(pcd_atk->m_id, pcd_def->m_id))
                            {
                                Singleton<relationMgr>::Instance().postEnemyInfos(pcd_atk->m_id,
                                                    "", pcd_def->m_id, pcd_def->m_name, ENEMY_NEWS_SERVANT_ENEMY_SUCCESS);
                            }

                            //增加仇恨
                            Singleton<relationMgr>::Instance().addHate(pCombat->m_defender->getCharId(), pCombat->m_attacker->getCharId(), 1);
                        }
                    }
                    else
                    {
                        pcs_atk->m_servant_list.push_back(pcs_def->m_cid);
                        pcs_atk->m_type = type_master;
                        pcs_def->m_master_id = pcs_atk->m_cid;
                        pcs_def->m_type = type_servant;
                        pcs_def->m_start_time = time(NULL);
                        pcs_def->m_end_time = pcs_def->m_start_time + 43200;
                        pcs_def->m_left = 72;
                        pcs_def->m_output = getServantOutput(pcs_def->m_cid)/6;
                        pcs_def->start();
                        //从可抓捕列表移除
                        RemoveFromCanList(pcs_atk->m_cid,pcs_def->m_cid);
                        std::string msg = strServantCatchSucAtk;
                        str_replace(msg, "$N", pcd_def->m_name);
                        str_replace(msg, "$N", pcd_def->m_name,true);
                        addServantEvent(pcs_atk->m_cid, msg);
                        msg = strServantCatchSucDef;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        addServantEvent(pcs_def->m_cid, msg);
                        //抓捕仇敌成功
                        if (Singleton<relationMgr>::Instance().is_my_enemy(pcd_atk->m_id, pcd_def->m_id))
                        {
                            Singleton<relationMgr>::Instance().postEnemyInfos(pcd_atk->m_id,
                                                "", pcd_def->m_id, pcd_def->m_name, ENEMY_NEWS_SERVANT_ENEMY_SUCCESS);
                        }
                    }
                    //抓捕成功玉石奖励
                    Item item_yushi;
                    item_yushi.type = item_type_treasure;
                    item_yushi.id = treasure_type_yushi;
                    item_yushi.nums = pcd_def->m_level / 2;
                    servantRealReward(item_yushi.nums);
                    pCombat->m_getItems.push_back(item_yushi);
                    //给东西
                    giveLoots(pcd_atk, pCombat, true, give_servant_loot);

                    //支线任务
                    pcd_atk->m_trunk_tasks.updateTask(task_arrest_servant, 1);
                }
                break;
            case 2://抢夺
                {
                    int pos = my_random(1,pcs_def->m_servant_list.size());
                    std::list<int>::iterator it = pcs_def->m_servant_list.begin();
                    while (it != pcs_def->m_servant_list.end())
                    {
                        --pos;
                        if (pos == 0)
                        {
                            boost::shared_ptr<charServant> pcs_change = GetCharServant(*it);
                            if (pcs_change.get())
                            {
                                int num = pcs_change->m_output * 80 / 100;
                                //int robnum = pcs_change->m_output * 80 / 100;
                                if ((pcs_def->m_get_num + num) >= getMax(*pcd_def))
                                {
                                    num = getMax(*pcd_def) - pcs_def->m_get_num;
                                }
                                if (num > 0)
                                {
                                    pcd_def->addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                                    gold_cost_tencent(pcd_def.get(),0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                                    add_statistics_of_treasure_cost(pcd_def->m_id, pcd_def->m_ip_address, treasure_type_yushi, num, treasure_servant, 1, pcd_def->m_union_id,pcd_def->m_server_id);
                                    pcs_def->m_get_num += num;
                                }
                                pcs_change->stop();
                                pcs_change->m_master_id = pcs_atk->m_cid;
                                pcs_change->m_type = type_servant;
                                pcs_change->m_start_time = time(NULL);
                                pcs_change->m_end_time = pcs_change->m_start_time + 43200;
                                pcs_change->m_left = 72;
                                pcs_change->m_output = getServantOutput(pcs_change->m_cid)/6;
                                pcs_change->start();
                                //从可抓捕列表移除
                                RemoveFromCanList(pcs_atk->m_cid,pcs_change->m_cid);
                                pcs_atk->m_servant_list.push_back(pcs_change->m_cid);
                                pcs_atk->m_type = type_master;
                                //cout << "rob suc!servant:" << pcs_change->m_cid << ",check(*it)=" << *it << " change master from " << pcs_def->m_cid << " to " << pcs_atk->m_cid << endl;
                                boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(pcs_change->m_cid);
                                if (!pcd.get())
                                    return HC_ERROR;
                                std::string msg = strServantRobSucAtk;
                                str_replace(msg, "$N", pcd_def->m_name);
                                str_replace(msg, "$N", pcd_def->m_name,true);
                                str_replace(msg, "$n", pcd->m_name);
                                str_replace(msg, "$n", pcd->m_name,true);
                                addServantEvent(pcs_atk->m_cid, msg);
                                msg = strServantRobSucDef;
                                str_replace(msg, "$N", pcd_atk->m_name);
                                str_replace(msg, "$N", pcd_atk->m_name,true);
                                str_replace(msg, "$n", pcd->m_name);
                                str_replace(msg, "$n", pcd->m_name,true);
                                addServantEvent(pcs_def->m_cid, msg);
                                if (!CheckCanList(pcs_def->m_cid,pcs_atk->m_cid))
                                    addServantEnemy(pcs_def->m_cid,pcs_atk->m_cid);
                                //抢夺成功只有基础产出
                                Item item_yushi;
                                item_yushi.type = item_type_treasure;
                                item_yushi.id = treasure_type_yushi;
                                item_yushi.nums = pcd_def->m_level / 2;
                                servantRealReward(item_yushi.nums);
                                pCombat->m_getItems.push_back(item_yushi);
                                //给东西
                                giveLoots(pcd_atk, pCombat, true, give_servant_loot);
                            }
                            break;
                        }
                        ++it;
                    }
                    //增加仇恨
                    Singleton<relationMgr>::Instance().addHate(pCombat->m_defender->getCharId(), pCombat->m_attacker->getCharId(), 1);
                }
                break;
            case 3://解救
                {
                    boost::shared_ptr<charServant> pcs_change = GetCharServant(pCombat->m_extra_data[1]);
                    if (pcs_change.get())
                    {
                        int num = pcs_change->m_output * 80 / 100;
                        if ((pcs_def->m_get_num + num) >= getMax(*pcd_def))
                        {
                            num = getMax(*pcd_def) - pcs_def->m_get_num;
                        }
                        if (num > 0)
                        {
                            pcd_def->addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                            gold_cost_tencent(pcd_def.get(),0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                            add_statistics_of_treasure_cost(pcd_def->m_id, pcd_def->m_ip_address, treasure_type_yushi, num, treasure_servant, 1, pcd_def->m_union_id,pcd_def->m_server_id);
                            pcs_def->m_get_num += num;
                        }
                        pcs_change->stop();
                    }
                    //支线任务
                    pcd_atk->m_trunk_tasks.updateTask(task_rescue_servant, 1);
                }
                break;
            case 10://反抗
                {
                    int num = pcs_atk->m_output * 80 / 100;
                    if ((pcs_def->m_get_num + num) >= getMax(*pcd_def))
                    {
                        num = getMax(*pcd_def) - pcs_def->m_get_num;
                    }
                    if (num > 0)
                    {
                        pcd_def->addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                        gold_cost_tencent(pcd_def.get(),0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                        add_statistics_of_treasure_cost(pcd_def->m_id, pcd_def->m_ip_address, treasure_type_yushi, num, treasure_servant, 1, pcd_def->m_union_id,pcd_def->m_server_id);
                        pcs_def->m_get_num += num;
                    }
                    pcs_atk->stop();
                    std::string msg = strServantResistSucAtk;
                    str_replace(msg, "$N", pcd_def->m_name);
                    str_replace(msg, "$N", pcd_def->m_name,true);
                    addServantEvent(pcs_atk->m_cid, msg);
                    msg = strServantResistSucDef;
                    str_replace(msg, "$N", pcd_atk->m_name);
                    str_replace(msg, "$N", pcd_atk->m_name,true);
                    addServantEvent(pcs_def->m_cid, msg);
                }
                break;
            case 11://求救
                {
                    boost::shared_ptr<charServant> pcs_change = GetCharServant(pCombat->m_extra_data[1]);
                    if (pcs_change.get())
                    {
                        int num = pcs_change->m_output * 80 / 100;
                        if ((pcs_def->m_get_num + num) >= getMax(*pcd_def))
                        {
                            num = getMax(*pcd_def) - pcs_def->m_get_num;
                        }
                        if (num > 0)
                        {
                            pcd_def->addTreasure(treasure_type_yushi, num);
#ifdef QQ_PLAT
                            gold_cost_tencent(pcd_def.get(),0,gold_cost_for_convert_jade,treasure_type_yushi,num);
#endif
                            add_statistics_of_treasure_cost(pcd_def->m_id, pcd_def->m_ip_address, treasure_type_yushi, num, treasure_servant, 1, pcd_def->m_union_id,pcd_def->m_server_id);
                            pcs_def->m_get_num += num;
                        }
                        pcs_change->stop();
                        std::string msg = strServantSOSSucAtk;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        addServantEvent(pcs_change->m_cid, msg);
                        msg = strServantSOSSucDef;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        str_replace(msg, "$n", pcd_atk->m_name);
                        str_replace(msg, "$n", pcd_atk->m_name,true);
                        addServantEvent(pcs_def->m_cid, msg);
                        boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(pcs_change->m_cid);
                        if (!pcd.get())
                            return HC_ERROR;
                        msg = strServantSOSSucHelper;
                        str_replace(msg, "$N", pcd->m_name);
                        str_replace(msg, "$N", pcd->m_name,true);
                        str_replace(msg, "$n", pcd_def->m_name);
                        str_replace(msg, "$n", pcd_def->m_name,true);
                        addServantEvent(pcs_atk->m_cid, msg);
                    }
                }
                break;
            default:
                break;
        }
        Save_data(pcs_atk->m_cid);
        Save_data(pcs_def->m_cid);
        Save_enemy_list(pcs_def->m_cid);
        Save_event_list(pcs_atk->m_cid);
        Save_event_list(pcs_def->m_cid);
    }
    else if(pCombat->m_state == defender_win)
    {
        boost::shared_ptr<CharData> pcd_atk = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
        boost::shared_ptr<CharData> pcd_def = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_defender->getCharId());
        if (!pcd_atk.get() || !pcd_def.get())
            return HC_ERROR;

        //战斗双方空闲了
        pcs_atk->m_fight_with = 0;
        pcs_def->m_fight_with = 0;
        switch(pCombat->m_extra_data[0])
        {
            case 1://抓捕
                {
                    if (pCombat->m_extra_data[1] != 0)
                    {
                        std::string msg = strServantRobFailAtk;
                        str_replace(msg, "$N", pcd_def->m_name);
                        str_replace(msg, "$N", pcd_def->m_name,true);
                        addServantEvent(pcs_atk->m_cid, msg);
                        msg = strServantRobFailDef;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        addServantEvent(pcs_def->m_cid, msg);

                        //抓捕仇敌失败
                        if (Singleton<relationMgr>::Instance().is_my_enemy(pcd_atk->m_id, pCombat->m_extra_data[1]))
                        {
                            boost::shared_ptr<CharData> ecdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_extra_data[1]);
                            Singleton<relationMgr>::Instance().postEnemyInfos(pcd_atk->m_id,
                                                "", ecdata->m_id, ecdata->m_name, ENEMY_NEWS_SERVANT_ENEMY_FAIL);
                        }
                    }
                    else
                    {
                        std::string msg = strServantCatchFailAtk;
                        str_replace(msg, "$N", pcd_def->m_name);
                        str_replace(msg, "$N", pcd_def->m_name,true);
                        addServantEvent(pcs_atk->m_cid, msg);
                        msg = strServantCatchFailDef;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        addServantEvent(pcs_def->m_cid, msg);

                        //抓捕仇敌失败
                        if (Singleton<relationMgr>::Instance().is_my_enemy(pcd_atk->m_id, pcd_def->m_id))
                        {
                            Singleton<relationMgr>::Instance().postEnemyInfos(pcd_atk->m_id,
                                                "", pcd_def->m_id, pcd_def->m_name, ENEMY_NEWS_SERVANT_ENEMY_FAIL);
                        }
                    }
                    //抓捕失败玉石奖励
                    Item item_yushi;
                    item_yushi.type = item_type_treasure;
                    item_yushi.id = treasure_type_yushi;
                    item_yushi.nums = pcd_atk->m_level / 5;
                    servantRealReward(item_yushi.nums);
                    pCombat->m_getItems.push_back(item_yushi);
                    //给东西
                    giveLoots(pcd_atk, pCombat, true, give_servant_loot);
                }
                break;
            case 2://抢夺
                {
                    std::string msg = strServantRobFailAtk;
                    str_replace(msg, "$N", pcd_def->m_name);
                    str_replace(msg, "$N", pcd_def->m_name,true);
                    addServantEvent(pcs_atk->m_cid, msg);
                    msg = strServantRobFailDef;
                    str_replace(msg, "$N", pcd_atk->m_name);
                    str_replace(msg, "$N", pcd_atk->m_name,true);
                    addServantEvent(pcs_def->m_cid, msg);
                }
                break;
            case 3://解救
                {
                }
                break;
            case 10://反抗
                {
                    std::string msg = strServantResistFailAtk;
                    str_replace(msg, "$N", pcd_def->m_name);
                    str_replace(msg, "$N", pcd_def->m_name,true);
                    addServantEvent(pcs_atk->m_cid, msg);
                    msg = strServantResistFailDef;
                    str_replace(msg, "$N", pcd_atk->m_name);
                    str_replace(msg, "$N", pcd_atk->m_name,true);
                    addServantEvent(pcs_def->m_cid, msg);
                }
                break;
            case 11://求救
                {
                    boost::shared_ptr<charServant> pcs_change = GetCharServant(pCombat->m_extra_data[1]);
                    if (pcs_change.get())
                    {
                        std::string msg = strServantSOSFailAtk;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        addServantEvent(pcs_change->m_cid, msg);
                        msg = strServantSOSFailDef;
                        str_replace(msg, "$N", pcd_atk->m_name);
                        str_replace(msg, "$N", pcd_atk->m_name,true);
                        str_replace(msg, "$n", pcd_atk->m_name);
                        str_replace(msg, "$n", pcd_atk->m_name,true);
                        addServantEvent(pcs_def->m_cid, msg);
                        boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(pcs_change->m_cid);
                        if (!pcd.get())
                            return HC_ERROR;
                        msg = strServantSOSFailHelper;
                        str_replace(msg, "$N", pcd->m_name);
                        str_replace(msg, "$N", pcd->m_name,true);
                        addServantEvent(pcs_atk->m_cid, msg);
                    }
                }
                break;
            default:
                break;
        }
        Save_event_list(pcs_atk->m_cid);
        Save_event_list(pcs_def->m_cid);
    }
    pCombat->AppendResult(pCombat->m_result_obj);
    //战报存/发送
    InsertSaveCombat(pCombat);
    return HC_SUCCESS;
}

void servantMgr::setFactor(int factor)
{
    if (factor < 100)
    {
        factor = 100;
    }
    m_servant_factor = factor;
    GeneralDataMgr::getInstance()->setInt("servant_event", m_servant_factor);    
}

