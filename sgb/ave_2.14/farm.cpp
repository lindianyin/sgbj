
#include "explore.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "spls_timer.h"
#include "statistics.h"
#include "daily_task.h"
#include "singleton.h"
#include "relation.h"
#include "statistics.h"

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern std::string strFarmMailContent;
extern std::string strFarmMailTitle;
extern std::string strCounts;

const int iSupplyEveryTime = 100;

//屯田实际收益
void farmRealReward(int& get);

#define INFO(x)

int field::start()
{
    INFO("field start farm" << endl);
    m_state = field_farm;
    end_nourish();
    //重启后的情况
    int leftsecond = m_end_time - time(NULL);
    if (leftsecond < 0)
        leftsecond = 0;
    int now_left_fights = leftsecond / 360;
    if (leftsecond % 360 != 0)
    {
        ++now_left_fights;
    }

    json_spirit::mObject mobj;
    mobj["cmd"] = "farmDone";
    mobj["cid"] = m_cid;
    mobj["pos"] = m_pos;
    boost::shared_ptr<splsTimer> tmsg;
    if (leftsecond <= 0 || m_left_num > now_left_fights)
    {
        //直接完成了
        tmsg.reset(new splsTimer(1, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    else
    {
        int start_first_time = leftsecond % 360;
        if (start_first_time == 0)
            start_first_time += 360;
        tmsg.reset(new splsTimer(start_first_time, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    save();
    return 0;
}

int field::done()
{
    ++m_reward_num;
    --m_left_num;
    //cout << m_cid << "," << m_pos << ",farm done,reward=" << m_reward_num << ",left=" << m_left_num << endl;
    if (m_left_num < 0)
        m_left_num = 0;
    if (m_left_num)
    {
        int leftsec = m_end_time - time(NULL);
        int now_left_fights = leftsec / 360;
        if (leftsec > 0 && leftsec % 360 != 0)
        {
            ++now_left_fights;
        }
        //新建下次定时器
        json_spirit::mObject mobj;
        mobj["cmd"] = "farmDone";
        mobj["cid"] = m_cid;
        mobj["pos"] = m_pos;
        //剩余时间不足以完成所有的次数
        if (leftsec <= 0 || m_left_num > now_left_fights)
        {
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(0.1, 1,mobj,1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
        else
        {
            boost::shared_ptr<splsTimer> tmsg;
            tmsg.reset(new splsTimer(360, 1,mobj,1));
            _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
        }
    }
    else
    {
        m_state = field_finish;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (cdata.get())
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getUpdateList") );
        obj.push_back( Pair("s", 200) );
        cdata->getUpdateListCD(obj);
        cdata->sendObj(obj);
    }
    save();
}

int field::save()
{
    InsertSaveDb("update char_farm_field set state=" + LEX_CAST_STR(m_state) +
        ",type=" + LEX_CAST_STR(m_type) +
        ",starttime=" + LEX_CAST_STR(m_start_time) +
        ",endtime=" + LEX_CAST_STR(m_end_time) +
        ",reward_num=" + LEX_CAST_STR(m_reward_num) +
        ",left_num=" + LEX_CAST_STR(m_left_num) +
        ",nourish_num=" + LEX_CAST_STR(m_nourish_num) +
        ",nourish_time=" + LEX_CAST_STR(m_nourish_time) +
        " where cid=" + LEX_CAST_STR(m_cid) +
        " and pos=" + LEX_CAST_STR(m_pos));
    return 0;
}

int field::get_nourish()
{
    int tmp = 0;
    time_t t_now = time(NULL);
    if (m_nourish_time < t_now && m_state == field_free)
    {
        tmp = (t_now - m_nourish_time) / 300 * 5;
    }
    return tmp + m_nourish_num;
}

void field::start_nourish()
{
    if (m_state == field_free)
    {
        m_nourish_time = time(NULL);
    }
    return;
}

void field::end_nourish()
{
    time_t t_now = time(NULL);
    if (m_state == field_farm && m_nourish_time < t_now)
    {
        m_nourish_num += (t_now - m_nourish_time) / 300 * 5;
        m_nourish_time = t_now;
    }
    return;
}

void field::clear_nourish()
{
    m_nourish_time = time(NULL);
    m_nourish_num = 0;
    save();
    return;
}

farmMgr* farmMgr::m_handle = NULL;

farmMgr* farmMgr::getInstance()
{
    if (NULL == m_handle)
    {
        time_t time_start = time(NULL);
        cout<<"farmMgr::getInstance()..."<<endl;
        m_handle = new farmMgr();
        m_handle->reload();
        cout<<"farmMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

int farmMgr::reload()
{
    Query q(GetDb());
    //load all activity farm_field
    std::list<uint64_t> cid_list;
    q.get_result("select distinct(cid) from char_farm_field where state=1 or state=4");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        cid_list.push_back(q.getval());
    }
    q.free_result();
    std::list<uint64_t>::iterator cit = cid_list.begin();
    while(cit != cid_list.end())
    {
        boost::shared_ptr<fieldlist> p_fl;
        p_fl.reset(new fieldlist);
        //load field
        q.get_result("select pos,state,type,starttime,endtime,reward_num,left_num,nourish_num,nourish_time from char_farm_field where cid=" + LEX_CAST_STR(*cit) + " order by pos");
        CHECK_DB_ERR(q);
        while(q.fetch_row())
        {
            boost::shared_ptr<field> p_f;
            p_f.reset(new field);
            p_f->m_cid = *cit;
            p_f->m_pos = q.getval();
            int state = q.getval();
            p_f->m_state = state;
            p_f->m_type = q.getval();
            p_f->m_start_time = q.getval();
            p_f->m_end_time = q.getval();
            p_f->m_reward_num = q.getval();
            p_f->m_left_num = q.getval();
            p_f->m_need_level = iFarmOpenLevel[p_f->m_pos - 1];
            p_f->m_supply = iSupplyEveryTime;
            p_f->m_nourish_num = q.getval();
            p_f->m_nourish_time = q.getval();
            if (p_f->m_nourish_time == 0 && state == field_free)
                p_f->m_nourish_time = time(NULL);
            (*p_fl).push_back(p_f);
            if (state == field_farm && p_f->m_left_num != 0)
            {
                p_f->m_nourish_time = time(NULL);
                p_f->start();
            }
        }
        q.free_result();
        m_char_field_list[*cit] = p_fl;
        ++cit;
    }
    //load water_list
    q.get_result("select cid,friend_id from char_farm_water where 1 order by cid");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        int cid = q.getval();
        int friend_id = q.getval();
        boost::shared_ptr<waterlist> p_wl = GetCharWaterList(cid);
        if (p_wl.get())
        {
            (*p_wl)[friend_id] = 1;
        }
    }
    return 0;
}

int farmMgr::resetAll()
{
    m_char_water_list.clear();
    InsertSaveDb("TRUNCATE TABLE char_farm_water");
    return HC_SUCCESS;
}

int farmMgr::FieldNum(int cid)
{
    int cnt = 0;
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get())
    {
        for (size_t i = 0; i < (*p_fl).size(); ++i)
        {
            if ((*p_fl)[i].get() && (*p_fl)[i]->m_state != field_lock)
            {
                ++cnt;
            }
        }
    }
    return cnt;
}

int farmMgr::StartFarmed(int cid, int pos, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    int now_seed = cdata->queryExtraData(char_data_type_daily, char_data_farm_seed);
    if (now_seed >= FieldNum(cid)*2)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get())
    {
        if (pos < 1 || pos > (*p_fl).size())
        {
            return HC_ERROR;
        }
        boost::shared_ptr<field> pf = (*p_fl)[pos - 1];
        INFO("check pos:" << pos << endl);
        if (pf.get() && pf->m_state == field_free)
        {
            pf->m_start_time = time(NULL);
            pf->m_end_time = pf->m_start_time + 240 * 60;
            pf->m_left_num = 240 / 6;
            pf->start();
            cdata->setExtraData(char_data_type_daily, char_data_farm_seed, ++now_seed);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int farmMgr::FarmDone(int cid, int pos)
{
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get() && pos <= (int)(*p_fl).size() && pos > 0)
    {
        boost::shared_ptr<field> p_f = (*p_fl)[pos - 1];
        if (p_f.get() && field_lock != p_f->m_state)
        {
            //完成
            p_f->done();
            return 0;
        }
    }
    return -1;
}

int farmMgr::SetFarmed(int cid, int pos, json_spirit::Object& robj)
{
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get() && pos > 0 && pos <= (int)p_fl->size() && (*p_fl)[pos - 1].get())
    {
        if ((*p_fl)[pos - 1]->m_reward_num <= 0)
        {
            return HC_ERROR;
        }
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (cdata.get())
        {
            INFO("add output pos:" << pos << endl);
            int add_reward = (*p_fl)[pos - 1]->m_supply * (*p_fl)[pos - 1]->m_reward_num;
            if ((*p_fl)[pos - 1]->m_type)
                add_reward = add_reward * 120 / 100;

            //屯田收益系数
            farmRealReward(add_reward);
            cdata->addTreasure(treasure_type_supply, add_reward);
            add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,treasure_type_supply,add_reward,treasure_farm,1,cdata->m_union_id,cdata->m_server_id);
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
            if (tr.get())
            {
                robj.push_back( Pair("msg", tr->name + strCounts + LEX_CAST_STR(add_reward)));
            }
            (*p_fl)[pos - 1]->m_reward_num = 0;
        }
        if ((*p_fl)[pos - 1]->m_reward_num == 0 && (*p_fl)[pos - 1]->m_left_num == 0)
        {
            (*p_fl)[pos - 1]->m_state = field_free;
            (*p_fl)[pos - 1]->start_nourish();
        }
        (*p_fl)[pos - 1]->save();
        cdata->NotifyCharData();
        //act统计
        act_to_tencent(cdata.get(),act_new_farm_get);

        //支线任务
        cdata->m_trunk_tasks.updateTask(task_farm_harvest, 1);
    }
    return HC_SUCCESS;
}

//加速
int farmMgr::speedFarm(int cid, int pos)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get())
    {
        if (pos < 1 || pos > (*p_fl).size())
        {
            return HC_ERROR;
        }
        if ((*p_fl)[pos - 1].get() && (*p_fl)[pos - 1]->m_state == field_farm)
        {
            int left_time = (*p_fl)[pos - 1]->m_end_time - time(NULL);
            int cost = left_time / 3600 * 10;
            if (left_time % 3600 > 0)
                cost += 10;
            if (left_time < 0 || cost < 0)
                cost = 0;
            if (cdata->addGold(-cost) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, cost, gold_cost_for_speed_farm, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(cdata.get(),cost,gold_cost_for_speed_farm);
#endif
            (*p_fl)[pos - 1]->m_state = field_finish;
            (*p_fl)[pos - 1]->m_reward_num += (*p_fl)[pos - 1]->m_left_num;
            (*p_fl)[pos - 1]->m_left_num = 0;
            (*p_fl)[pos - 1]->m_end_time = 0;
            splsTimerMgr::getInstance()->delTimer((*p_fl)[pos - 1]->_uuid);
            (*p_fl)[pos - 1]->save();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//调试用立即完成
int farmMgr::updateAll(int cid)
{
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get())
    {
        for (int pos = 1; pos <= 6; ++pos)
        {
            if ((*p_fl)[pos - 1].get() && (*p_fl)[pos - 1]->m_state == field_farm)
            {
                //(*p_fl)[pos - 1]->m_left_min = 0;
                splsTimerMgr::getInstance()->delTimer((*p_fl)[pos - 1]->_uuid);
                json_spirit::mObject mobj;
                mobj["cmd"] = "farmDone";
                mobj["cid"] = (*p_fl)[pos - 1]->m_cid;
                mobj["pos"] = (*p_fl)[pos - 1]->m_pos;
                boost::shared_ptr<splsTimer> tmsg;
                tmsg.reset(new splsTimer(1, 1,mobj,1));
                (*p_fl)[pos - 1]->_uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
                (*p_fl)[pos - 1]->save();
            }
        }
    }
    return HC_SUCCESS;
}

int farmMgr::UpFarmField(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<fieldlist> p_fl = GetCharFieldList(cid);
    if (p_fl.get())
    {
        for (int i = 0; i < (*p_fl).size(); ++i)
        {
            boost::shared_ptr<field> pf = (*p_fl)[i];
            if (pf.get() && pf->m_type == 0)
            {
                if (-1 == cdata->addGold(-200))
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                //金币消耗统计
                add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, 200, gold_cost_for_upgrade_farm, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
                gold_cost_tencent(cdata.get(),200,gold_cost_for_upgrade_farm);
#endif
                pf->m_type = 1;
                pf->save();
                robj.push_back( Pair("pos", i + 1) );
                cdata->NotifyCharData();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

int farmMgr::WaterFarmField(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int now_water = cdata->queryExtraData(char_data_type_daily, char_data_farm_water);
    if (now_water > iFarmWater)
    {
        return HC_ERROR;
    }
    int cd = cdata->queryExtraData(char_data_type_daily, char_data_farm_water_cd);
    if (cd > time(NULL))
    {
        return HC_ERROR;
    }
    int add_reward = cdata->m_level / 5;
    if (cdata->m_level % 5 > 0)
        ++add_reward;
    //军粮数值放大10倍
    if (add_reward > 0)
        add_reward *= 10;
    cdata->addTreasure(treasure_type_supply, add_reward);
    add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,treasure_type_supply,add_reward,treasure_farm_water,1,cdata->m_union_id,cdata->m_server_id);
    cdata->setExtraData(char_data_type_daily, char_data_farm_water, ++now_water);
    cdata->setExtraData(char_data_type_daily, char_data_farm_water_cd, time(NULL)+1800);
    boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
    if (tr.get())
    {
        robj.push_back( Pair("msg", tr->name + strCounts + LEX_CAST_STR(add_reward)));
    }
    cdata->NotifyCharData();
    //act统计
    act_to_tencent(cdata.get(),act_new_water);

    //支线任务
    cdata->m_trunk_tasks.updateTask(task_farm_water, 1);
    return HC_SUCCESS;
}

bool farmMgr::getWaterState(int cid, int friend_id)
{
    boost::shared_ptr<waterlist> p_wl = GetCharWaterList(cid);
    if (p_wl.get())
    {
        return p_wl->find(friend_id) != p_wl->end();
    }
    return false;
}

void farmMgr::setWaterState(int cid, int friend_id)
{
    boost::shared_ptr<waterlist> p_wl = GetCharWaterList(cid);
    if (p_wl.get())
    {
        (*p_wl)[friend_id] = 1;
        InsertSaveDb("replace into char_farm_water (cid,friend_id) values ("
                + LEX_CAST_STR(cid)
                + "," + LEX_CAST_STR(friend_id)
                + ")");
    }
    return;
}

int farmMgr::WaterFarmFriendField(int cid, int friend_id, json_spirit::Object& robj)
{
    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cid);
     if (my_rl.get() && my_rl->is_attention(friend_id) && !getWaterState(cid,friend_id))
     {
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
        if (!cdata.get())
            return HC_ERROR;
        int now_water = cdata->queryExtraData(char_data_type_daily, char_data_farm_friend_water);
        if (now_water >= iFarmFriendWater)
        {
            return HC_ERROR_FARM_WATER_TIMES;
        }
        int add_reward = cdata->m_level / 10;
        if (cdata->m_level % 10 > 0)
            ++add_reward;
        //军粮数值放大10倍
        if (add_reward > 0)
            add_reward *= 10;
        
        //屯田收益系数
        farmRealReward(add_reward);
        cdata->addTreasure(treasure_type_supply, add_reward);
        add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,treasure_type_supply,add_reward,treasure_farm_water,1,cdata->m_union_id,cdata->m_server_id);
        cdata->setExtraData(char_data_type_daily, char_data_farm_friend_water, ++now_water);
        setWaterState(cid,friend_id);
        cdata->NotifyCharData();
        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
        if (tr.get())
        {
            robj.push_back( Pair("msg", tr->name + strCounts + LEX_CAST_STR(add_reward)));
        }

        //支线任务
        cdata->m_trunk_tasks.updateTask(task_farm_water, 1);
        return HC_SUCCESS;
     }
    return HC_ERROR;
}

int farmMgr::WaterFarmFriendFieldAll(int cid, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    int now_water = cdata->queryExtraData(char_data_type_daily, char_data_farm_friend_water);
    if (now_water >= iFarmFriendWater)
    {
        return HC_ERROR_FARM_WATER_TIMES;
    }
    int cnt = iFarmFriendWater - now_water;
    int tmp_cnt = 0;
    boost::shared_ptr<char_ralation> my_rl = Singleton<relationMgr>::Instance().getRelation(cdata->m_id);
    json_spirit::Array list;
    for (std::map<int, boost::shared_ptr<char_ralation> >::iterator it = my_rl->m_my_attentions.begin();
            it != my_rl->m_my_attentions.end();
                ++it)
    {
        if (cnt <= 0)
        {
            setWaterState(cid,it->second->m_char_id);
        }
        else if (!getWaterState(cid,it->second->m_char_id))
        {
            //json_spirit::Object o;
            //WaterFarmFriendField(cid,it->second->m_char_id,o);
            --cnt;
            ++tmp_cnt;
            setWaterState(cid,it->second->m_char_id);
            //cout << "water friend_id=" << it->second->m_char_id << endl;
        }
    }
    if (tmp_cnt == 0)
    {
        return HC_ERROR_NO_FARM_WATER_FRIEND;
    }
    int add_reward = cdata->m_level / 10;
    if (cdata->m_level % 10 > 0)
        ++add_reward;
    //军粮数值放大10倍
    if (add_reward > 0)
        add_reward *= 10;
    add_reward *= tmp_cnt;
    
    //屯田收益系数
    farmRealReward(add_reward);
    cdata->addTreasure(treasure_type_supply, add_reward);
    add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,treasure_type_supply,add_reward,treasure_farm_water,1,cdata->m_union_id,cdata->m_server_id);
    cdata->NotifyCharData();
    
    //支线任务
    cdata->m_trunk_tasks.updateTask(task_farm_water, tmp_cnt);
    
    cdata->setExtraData(char_data_type_daily, char_data_farm_friend_water, now_water+tmp_cnt);
    //cout << "water_cnt=" << tmp_cnt;
    boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
    if (tr.get())
    {
        robj.push_back( Pair("msg", tr->name + strCounts + LEX_CAST_STR(add_reward)));
    }
    return HC_SUCCESS;
}

boost::shared_ptr<fieldlist> farmMgr::GetCharFieldList(int cid)
{
    boost::shared_ptr<fieldlist> p_fl;
    std::map<int, boost::shared_ptr<fieldlist> >::iterator it =  m_char_field_list.find(cid);
    if (it != m_char_field_list.end())
    {
        return it->second;
    }
    else
    {
        p_fl.reset(new fieldlist);
        INFO("load charfieldlist from db" << endl);
        Query q(GetDb());
        q.get_result("select pos,state,type,starttime,endtime,reward_num,left_num,nourish_num,nourish_time from char_farm_field where cid=" + LEX_CAST_STR(cid) + " order by pos");
        CHECK_DB_ERR(q);
        while(q.fetch_row())
        {
            boost::shared_ptr<field> p_f;
            p_f.reset(new field);
            p_f->m_cid = cid;
            p_f->m_pos = q.getval();
            int state = q.getval();
            p_f->m_state = state;
            p_f->m_type = q.getval();
            p_f->m_start_time = q.getval();
            p_f->m_end_time = q.getval();
            p_f->m_reward_num = q.getval();
            p_f->m_left_num = q.getval();
            p_f->m_need_level = iFarmOpenLevel[p_f->m_pos - 1];
            p_f->m_supply = iSupplyEveryTime;
            p_f->m_nourish_num = q.getval();
            p_f->m_nourish_time = q.getval();
            if (p_f->m_nourish_time == 0 && state == field_free)
                p_f->m_nourish_time = time(NULL);
            (*p_fl).push_back(p_f);
            if (state == field_farm)
            {
                p_f->m_nourish_time = time(NULL);
                p_f->start();
            }
        }
        q.free_result();
        m_char_field_list[cid] = p_fl;
        if (p_fl->size() == 0)
        {
            boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
            if (!cdata.get())
                return p_fl;
            if (cdata->m_currentStronghold >= iFarmOpenStronghold[0])
                return open(cid);
        }
        INFO("load charfieldlist success" << endl);
        return p_fl;
    }
    p_fl.reset();
    return p_fl;
}

boost::shared_ptr<waterlist> farmMgr::GetCharWaterList(int cid)
{
    boost::shared_ptr<waterlist> p_fl;
    std::map<int, boost::shared_ptr<waterlist> >::iterator it =  m_char_water_list.find(cid);
    if (it != m_char_water_list.end())
    {
        return it->second;
    }
    else
    {
        p_fl.reset(new waterlist);
        m_char_water_list[cid] = p_fl;
        return p_fl;
    }
    p_fl.reset();
    return p_fl;
}

boost::shared_ptr<fieldlist> farmMgr::open(int cid)
{
    std::map<int, boost::shared_ptr<fieldlist> >::iterator it =  m_char_field_list.find(cid);
    if (it != m_char_field_list.end() && it->second->size() != 0)
    {
        return it->second;
    }
    boost::shared_ptr<fieldlist> p_fl;
    p_fl.reset(new fieldlist);
    //屯田开启
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return p_fl;
    for (int i = 1; i <= 6; ++i)
    {
        int state = field_lock;
        //等级判断
        if (cdata->m_currentStronghold >= iFarmOpenStronghold[i - 1])
            state = field_free;
        boost::shared_ptr<field> p_f;
        p_f.reset(new field);
        p_f->m_cid = cid;
        p_f->m_pos = i;
        p_f->m_state = state;
        p_f->m_type = 0;
        p_f->m_start_time = 0;
        p_f->m_end_time = 0;
        p_f->m_reward_num = 0;
        p_f->m_left_num = 0;
        p_f->m_need_level = iFarmOpenLevel[i - 1];
        p_f->m_supply = iSupplyEveryTime;
        if (p_f->m_state == field_free)
            p_f->start_nourish();
        (*p_fl).push_back(p_f);
        InsertSaveDb("insert into char_farm_field (cid,pos,state,nourish_time) values (" + LEX_CAST_STR(cid) + "," + LEX_CAST_STR(i) + "," + LEX_CAST_STR(state) + ",unix_timestamp())");
    }
    m_char_field_list[cid] = p_fl;
    return p_fl;
}

int farmMgr::deleteChar(int cid)
{
    InsertSaveDb("delete from char_farm_field where cid=" + LEX_CAST_STR(cid));
    m_char_field_list.erase(cid);
    return HC_SUCCESS;
}

int farmMgr::getCoolTime(int cid, int& state)
{
    int lefttime = 0;
    time_t timenow = time(NULL);
    boost::shared_ptr<fieldlist> fl = farmMgr::getInstance()->GetCharFieldList(cid);
    if (fl.get())
    {
        bool betired = true;
        for (size_t j = 0; j < (*fl).size(); ++j)
        {
            if (!(*fl)[j].get())
                continue;
            if ((*fl)[j]->m_state == 5)
            {
                continue;
            }
            else if((*fl)[j]->m_state != 4)
            {
                if (lefttime == 0 || ((*fl)[j]->m_end_time - timenow) < lefttime)
                {
                    lefttime = (*fl)[j]->m_end_time - timenow;
                }
                betired = false;
                if (lefttime <= 0)
                {
                    break;
                }
            }
        }
        if (lefttime < 0)
        {
            lefttime = 0;
        }
        if (lefttime == 0)
        {
            if (!betired)
            {
                state = 1;
            }
            else
            {
                state = 3;
            }
        }
        else
        {
            state = 2;
        }
    }
    return lefttime;
}

int farmMgr::getRewardTimes(int cid)
{
    int rewardtimes = 0;
    boost::shared_ptr<fieldlist> fl = farmMgr::getInstance()->GetCharFieldList(cid);
    if (fl.get())
    {
        for (size_t j = 0; j < (*fl).size(); ++j)
        {
            if ((*fl)[j].get())
                rewardtimes += (*fl)[j]->m_reward_num;
        }
    }
    return rewardtimes;
}

int farmMgr::getNourishReward(int cid)
{
    int reward = 0;
    int max_reward = FieldNum(cid) * 1440;
    boost::shared_ptr<fieldlist> fl = farmMgr::getInstance()->GetCharFieldList(cid);
    if (fl.get())
    {
        for (size_t j = 0; j < (*fl).size(); ++j)
        {
            if ((*fl)[j].get())
                reward += (*fl)[j]->get_nourish();
        }
    }
    if (reward > max_reward)
        reward = max_reward;
    
    //屯田收益系数
    farmRealReward(reward);
    return reward;
}

void farmMgr::clearNourishReward(int cid)
{
    boost::shared_ptr<fieldlist> fl = farmMgr::getInstance()->GetCharFieldList(cid);
    if (fl.get())
    {
        for (size_t j = 0; j < (*fl).size(); ++j)
        {
            if ((*fl)[j].get())
                (*fl)[j]->clear_nourish();
        }
    }
    return;
}

int farmMgr::rewardNourish(int cid, json_spirit::Object& robj)
{
    int reward = getNourishReward(cid);
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (cdata.get())
    {
        cdata->addTreasure(treasure_type_supply, reward);
        cdata->NotifyCharData();
        add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,treasure_type_supply,reward,treasure_farm,1,cdata->m_union_id,cdata->m_server_id);
        clearNourishReward(cid);
        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_supply);
        if (tr.get())
        {
            robj.push_back( Pair("msg", tr->name + strCounts + LEX_CAST_STR(reward)));
        }
        //act统计
        act_to_tencent(cdata.get(),act_new_nourish);

        //支线任务
        cdata->m_trunk_tasks.updateTask(task_farm_yechan, 1);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

