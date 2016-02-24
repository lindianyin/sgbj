
#include "sweep.h"
#include "utils_all.h"
#include "utils_lang.h"
#include "errcode_def.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_timer.h"
#include "const_def.h"
#include "SaveDb.h"
#include "copy.h"

class chessCombat;
extern std::string strSweepMailTitle;
extern std::string strSweepMailContent;

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern int InsertInternalActionWork(json_spirit::mObject& obj);

//扫荡的处理
int ProcessSweepDone(json_spirit::mObject& o)
{
    int cid = 0;
    READ_INT_FROM_MOBJ(cid, o, "cid");
    Singleton<sweepMgr>::Instance().done(cid);
    return HC_SUCCESS;
}

//开始扫荡
int ProcessSweep(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    if (cdata->m_level < iSweepOpenLevel && cdata->m_vip < iSweepOpenVip)
    {
        return HC_ERROR_NEED_MORE_VIP;
    }
    int copyid = 0;
    READ_INT_FROM_MOBJ(copyid, o, "id");
    boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(copyid);
    if (!bc.get())
    {
        return HC_ERROR;
    }
    if (cdata->m_level < bc->m_openLevel)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
    if (has_attack > iCopyTotal)
    {
        return HC_ERROR_NOT_ENOUGH_TIMES;
    }
    boost::shared_ptr<CharCopyData> cd = Singleton<copyMgr>::Instance().getCharCopy(cdata->m_id,bc->m_mapid,bc->m_id);
    if (cd.get())
    {
        if (cd->m_can_attack <= 0)
        {
            return HC_ERROR_NOT_ENOUGH_TIMES;
        }
        if (cd->m_result == 0)
        {
            return HC_ERROR;
        }
    }
    boost::shared_ptr<charSweep> pcs = Singleton<sweepMgr>::Instance().getCharSweepData(cdata->m_id);
    if (pcs.get() && pcs->m_sweep_id == 0)
    {
        pcs->m_sweep_id = copyid;
        //robj.push_back( Pair("sweep_type", pcs->m_type));
        if (pcs->m_end_time == 0)//开始扫荡
        {
            //包满无法扫荡
            if (cdata->m_bag.isFull())
            {
                return HC_ERROR_BAG_FULL;
            }
            if (cdata->m_vip < iSweepFinishVip)
            {
                pcs->m_fast_mod = 0;
                pcs->m_start_time = time(NULL);
                pcs->m_end_time = pcs->m_start_time + iSweepTime;
                robj.push_back( Pair("leftTime", iSweepTime));
            }
            else
            {
                pcs->m_start_time = 1;
                pcs->m_end_time = 1;
                pcs->m_fast_mod = 1;
            }
            return pcs->start();
        }
        else
        {
            return HC_ERROR;
        }
    }
    else
    {
        return HC_ERROR;
    }
    return HC_SUCCESS;
}

int charSweep::start()
{
    if (m_sweep_id <= 0)
    {
        m_start_time = 0;
        m_end_time = 0;
        ERR();
        return HC_ERROR;
    }
    if (1 == m_fast_mod)
    {
        json_spirit::mObject mobj;
        mobj["cmd"] = "sweepDone";
        mobj["cid"] = m_cid;
        InsertInternalActionWork(mobj);
        return HC_SUCCESS;
    }
    else
    {
        //重启后的情况
        int leftsecond = m_end_time - time(NULL);
        if (leftsecond <= 0)
        {
            json_spirit::mObject mobj;
            mobj["cmd"] = "sweepDone";
            mobj["cid"] = m_cid;
            InsertInternalActionWork(mobj);
            return HC_SUCCESS;
        }
        else
        {
            json_spirit::mObject mobj;
            mobj["cmd"] = "sweepDone";
            mobj["cid"] = m_cid;
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(leftsecond, 1,mobj,1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
    }
    save();
    return HC_SUCCESS;
}

int charSweep::speedup()
{
    if (m_sweep_id <= 0)
    {
        return HC_ERROR;
    }
    int left_sec = m_end_time - time(NULL);
    if (left_sec > 0)
    {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        if (!cdata.get())
            return HC_ERROR;
        int cost_gold = left_sec / 60;
        if (left_sec % 60 > 0)
            ++cost_gold;
        if (cost_gold > 0)
        {
            if (cdata->subGold(cost_gold, gold_cost_sweep_cd) == -1)
                return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        if (!_uuid.is_nil())
        {
            splsTimerMgr::getInstance()->delTimer(_uuid);
            _uuid = boost::uuids::nil_uuid();
        }
        m_fast_mod = 1;
        int ret = done();
        if (ret != HC_SUCCESS)
        {
            //通知客户端
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
            if (account.get())
            {
                json_spirit::Object obj;
                obj.clear();
                obj.push_back( Pair("cmd", "sweepDone") );
                obj.push_back( Pair("s", ret) );
                account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
            }
        }
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int charSweep::done()
{
    if ((0 == m_fast_mod && 0 == m_start_time) || m_sweep_id <= 0)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<baseCopy> bc = Singleton<copyMgr>::Instance().getCopyById(m_sweep_id);
    if (!bc.get())
    {
        return HC_ERROR;
    }
    if (cdata->m_level < bc->m_openLevel)
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    int has_attack = cdata->queryExtraData(char_data_type_daily,char_data_daily_attack_copy);
    if (has_attack > iCopyTotal)
    {
        return HC_ERROR_NOT_ENOUGH_TIMES;
    }
    boost::shared_ptr<CharCopyData> cd = Singleton<copyMgr>::Instance().getCharCopy(cdata->m_id,bc->m_mapid,bc->m_id);
    if (cd.get())
    {
        if (cd->m_can_attack <= 0)
        {
            return HC_ERROR_NOT_ENOUGH_TIMES;
        }
        if (cd->m_result == 0)
        {
            return HC_ERROR;
        }
        cdata->setExtraData(char_data_type_daily,char_data_daily_attack_copy, ++has_attack);
        --cd->m_can_attack;
        cd->save();
        /***********随机获得掉落处理****************/
        itemlist.clear();
        Singleton<lootMgr>::Instance().getCopyLoots(m_sweep_id, itemlist, cd->m_result);
        //给东西
        std::list<Item> Item_list = itemlist;
        giveLoots(cdata.get(), Item_list,NULL,NULL,true,loot_copy);
    }
    //发送信件
    std::string content = strSweepMailContent;
    int cost_min = (m_end_time - m_start_time) / 60;
    if (cost_min < 0)
        cost_min = (time(NULL) - m_start_time) / 60;
    str_replace(content, "$T", LEX_CAST_STR(cost_min));
    std::string reward = "";
    int silver_total = 0;
    std::map<int,int> gem_list,equipment_list;
    std::list<Item>::iterator it = itemlist.begin();
    while (it != itemlist.end())
    {
        switch ((*it).type)
        {
            case ITEM_TYPE_CURRENCY:
            {
                if ((*it).id == CURRENCY_ID_SILVER)
                {
                    silver_total += (*it).nums;
                }
                break;
            }
            case ITEM_TYPE_GEM:
            {
                std::map<int,int>::iterator it_m = gem_list.find((*it).id);
                if (it_m != gem_list.end())
                {
                    it_m->second += (*it).nums;
                }
                else
                    gem_list[(*it).id] = (*it).nums;
                break;
            }
            case ITEM_TYPE_EQUIPMENT://装备
            {
                std::map<int,int>::iterator it_m = equipment_list.find((*it).id);
                if (it_m != equipment_list.end())
                {
                    it_m->second += (*it).nums;
                }
                else
                    equipment_list[(*it).id] = (*it).nums;
                break;
            }
        }
        ++it;
    }
    std::map<int,int>::iterator it_reward;
    if (!gem_list.empty())
    {
        it_reward = gem_list.begin();
        while (it_reward != gem_list.end())
        {
            boost::shared_ptr<baseGem> tr = GeneralDataMgr::getInstance()->GetBaseGem(it_reward->first);
            if (tr.get())
            {
                reward += (tr->name + strCounts + LEX_CAST_STR(it_reward->second) + " ");
            }
            ++it_reward;
        }
    }
    if (!equipment_list.empty())
    {
        it_reward = equipment_list.begin();
        while (it_reward != equipment_list.end())
        {
            boost::shared_ptr<baseEquipment> tr = GeneralDataMgr::getInstance()->GetBaseEquipment(it_reward->first);
            if (tr.get())
            {
                reward += (tr->name + strCounts + LEX_CAST_STR(it_reward->second) + " ");
            }
            ++it_reward;
        }
    }
    if (silver_total != 0)
        reward += (strSilver + strCounts + LEX_CAST_STR(silver_total) + " ");
    str_replace(content, "$R", reward);
    //通知客户端完成扫荡
    boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
    if (account.get())
    {
        json_spirit::Object obj;
        obj.clear();
        obj.push_back( Pair("cmd", "sweepDone") );
        obj.push_back( Pair("s", 200) );
        account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
    }
    sendSystemMail(cdata->m_name, m_cid, strSweepMailTitle, content);
    stop();
    return HC_SUCCESS;
}

int charSweep::stop()
{
    if (m_start_time > 0 && m_end_time > 0)
    {
        m_sweep_id = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_fast_mod = 0;
        if (!_uuid.is_nil())
        {
            splsTimerMgr::getInstance()->delTimer(_uuid);
            _uuid = boost::uuids::nil_uuid();
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

void charSweep::save()
{
    InsertSaveDb("delete from char_sweep where cid=" + LEX_CAST_STR(m_cid));
    if (m_fast_mod == 0 && m_sweep_id > 0)
    {
        InsertSaveDb("insert into char_sweep set cid=" + LEX_CAST_STR(m_cid)
            + ", start_time=" + LEX_CAST_STR(m_start_time)
            + ", end_time=" + LEX_CAST_STR(m_end_time)
            + ", sweep_id=" + LEX_CAST_STR(m_sweep_id));
    }
}

sweepMgr::sweepMgr()
{
}

int sweepMgr::speedUp(int cid)
{
    boost::shared_ptr<charSweep> pcs = getCharSweepData(cid);
    if (pcs.get())
    {
        int ret = pcs->speedup();
        if (ret == HC_SUCCESS)
            pcs->save();
        return ret;
    }
    return HC_ERROR;
}

int sweepMgr::done(int cid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<charSweep> pcs = getCharSweepData(cid);
    if (pcs.get())
    {
        int ret = pcs->done();
        pcs->save();
        if (ret != HC_SUCCESS)
        {
            //cout << "send sweepdone!!" << endl;
            //通知客户端
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
            if (account.get())
            {
                json_spirit::Object obj;
                obj.clear();
                obj.push_back( Pair("cmd", "sweepDone") );
                obj.push_back( Pair("s", ret) );
                account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
            }
        }
        return 0;
    }
    return -1;
}

boost::shared_ptr<charSweep> sweepMgr::getCharSweepData(int cid)
{
    std::map<int,boost::shared_ptr<charSweep> >::iterator it = m_sweep_task.find(cid);
    if (it != m_sweep_task.end())
    {
        return it->second;
    }
    boost::shared_ptr<charSweep> p;
    p.reset(new charSweep(cid));
    Query q(GetDb());
    q.get_result("SELECT start_time,end_time,sweep_id FROM char_sweep WHERE cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        p->m_start_time = q.getval();
        p->m_end_time = q.getval();
        if (p->m_end_time < p->m_start_time)
        {
            p->m_end_time = p->m_start_time;
        }
        p->m_sweep_id = q.getval();
        //p->m_type = q.getval();
    }
    q.free_result();
    m_sweep_task[cid] = p;
    if (p->m_sweep_id > 0)
        p->start();
    return p;
}

