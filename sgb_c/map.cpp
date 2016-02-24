
#include "map.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "errcode_def.h"
#include "utils_all.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "SaveDb.h"
#include "spls_timer.h"
#include "data.h"
#include "loot.h"
#include "action.h"
#include "utils_lang.h"
#include "relation.h"
#include "weekRanking.h"

Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

//显示玩家地图场景进度
int ProcessCharMapTempo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharTempoData& char_tempo = pc->m_tempo;
    int mapid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "id");
    //请求地图大于当前地图判断下是否需要更新进度
    if (mapid > pc->m_cur_mapid)
    {
        if (pc->m_cur_mapid >= iMaxMapid)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(pc->m_cur_mapid + 1);
        if (!bm.get())
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        //通关才能进入下一图
        if (char_tempo.isMapPassed(pc->m_cur_mapid))
        {
            ++pc->m_cur_mapid;
            pc->m_cur_stageid = 1;
            char_tempo.InitCharTempo(pc->m_cur_mapid);
            pc->Save();
            //cout<<"auto move to next map..."<<endl;
        }
        else
        {
            //cout<<"!!! auto move to next map fail..."<<endl;
            return HC_ERROR_MAP_NOT_OPEN;
        }
    }
    //场景列表
    json_spirit::Array maptempo;
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
    if (it != char_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).begin();
        while (itm != (*md).end())
        {
            if (itm->second.get() && itm->second->m_baseStage.get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("id", itm->first));
                obj.push_back( Pair("spic", itm->second->m_baseStage->m_spic));
                obj.push_back( Pair("name", itm->second->m_baseStage->m_name));
                obj.push_back( Pair("minlevel", itm->second->m_baseStage->m_minlevel));
                obj.push_back( Pair("maxlevel", itm->second->m_baseStage->m_maxlevel));
                if (itm->second->m_baseStage->m_size > 0 && itm->second->m_strongholds[0].get() && itm->second->m_strongholds[itm->second->m_baseStage->m_size - 1].get())
                {
                    if (itm->second->m_strongholds[itm->second->m_baseStage->m_size - 1]->isPassed())
                    {
                        obj.push_back( Pair("state", 2));
                    }
                    else if (itm->second->m_strongholds[0]->m_state > -1)
                    {
                        obj.push_back( Pair("state", 1));
                    }
                    else
                    {
                        obj.push_back( Pair("state", 0));
                    }
                }
                else
                {
                    obj.push_back( Pair("state", 0));
                }
                maptempo.push_back(obj);
            }
            ++itm;
        }
    }
    robj.push_back( Pair("stage_list", maptempo));
    //全部地图列表
    json_spirit::Array maplist;
    for (int i = 1; i <= iMaxMapid; ++i)
    {
        std::string name = "";
        std::string memo = "";
        if (0 == Singleton<mapMgr>::Instance().GetMapMemo(i,name,memo))
        {
            json_spirit::Object mapinfo;
            mapinfo.push_back( Pair("id", i));
            mapinfo.push_back( Pair("name", name));
            mapinfo.push_back( Pair("memo", memo));
            maplist.push_back(mapinfo);
            if (mapid == i)
            {
                robj.push_back( Pair("cur_map", mapinfo));
            }
        }
    }
    robj.push_back( Pair("map_list", maplist));
    return HC_SUCCESS;
}

//显示玩家场景关卡进度
int ProcessCharStageTempo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    CharTempoData& char_tempo = pc->m_tempo;
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");

    //请求地图大于当前地图判断下是否需要更新进度
    if (mapid > pc->m_cur_mapid)
    {
        if (pc->m_cur_mapid >= iMaxMapid)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(pc->m_cur_mapid + 1);
        if (!bm.get())
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        //通关才能进入下一图
        if (char_tempo.isMapPassed(pc->m_cur_mapid))
        {
            ++pc->m_cur_mapid;
            pc->m_cur_stageid = 1;
            char_tempo.InitCharTempo(pc->m_cur_mapid);
            pc->Save();
            //cout<<"auto move to next map..."<<endl;
        }
        else
        {
            //cout<<"!!! auto move to next map fail..."<<endl;
            return HC_ERROR_MAP_NOT_OPEN;
        }
    }
    //关卡列表
    json_spirit::Array stronghold_array;
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = char_tempo.CharMapsData.find(mapid);
    if (it != char_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).find(stageid);
        if (itm != (*md).end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            robj.push_back( Pair("name", itm->second->m_baseStage->m_name));
            for (size_t i = 0; i < (size_t)itm->second->m_baseStage->m_size; ++i)
            {
                if (!itm->second->m_strongholds[i].get() || !itm->second->m_strongholds[i]->m_baseStronghold.get())
                {
                    continue;
                }
                baseStronghold* shold = itm->second->m_strongholds[i]->m_baseStronghold.get();
                if (shold)
                {
                    json_spirit::Object stronghold;
                    stronghold.push_back( Pair("pos", i + 1));
                    stronghold.push_back( Pair("id", shold->m_id));
                    stronghold.push_back( Pair("name", shold->m_name));
                    stronghold.push_back( Pair("level", shold->m_level));
                    stronghold.push_back( Pair("spic", shold->m_spic));
                    stronghold.push_back( Pair("type", shold->m_type));
                    stronghold.push_back( Pair("x", shold->m_station.m_x));
                    stronghold.push_back( Pair("y", shold->m_station.m_y));
                    stronghold.push_back( Pair("state", itm->second->m_strongholds[i]->state()));
                    stronghold.push_back( Pair("cur_times", itm->second->m_strongholds[i]->m_state));
                    stronghold.push_back( Pair("need_times", shold->m_need_times));
                    json_spirit::Array path;
                    for(std::vector<Pos>::iterator it = shold->m_path.begin(); it != shold->m_path.end(); ++it)
                    {
                        json_spirit::Object o;
                        o.push_back( Pair("x", (*it).m_x));
                        o.push_back( Pair("y", (*it).m_y));
                        path.push_back(o);
                    }
                    stronghold.push_back( Pair("path", path));
                    stronghold_array.push_back(stronghold);
                }
            }
        }
    }
    robj.push_back( Pair("stronghold_list", stronghold_array));
    robj.push_back( Pair("mapid", mapid));
    robj.push_back( Pair("stageid", stageid));
    return HC_SUCCESS;
}

//显示玩家单个关卡信息
int ProcessCharStronghold(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int strongholdid = 0;
    READ_INT_FROM_MOBJ(strongholdid, o, "strongholdid");
    int mapid = 0, stageid = 0, strongholdpos = 0;
    strongholdpos = Singleton<mapMgr>::Instance().GetStrongholdPos(mapid,stageid,strongholdid);
    if (mapid == 0 || stageid == 0 || strongholdpos == 0)
    {
        return HC_ERROR;
    }

    //请求地图大于当前地图判断下是否需要更新进度
    if (mapid > pc->m_cur_mapid)
    {
        if (pc->m_cur_mapid >= iMaxMapid)
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(pc->m_cur_mapid + 1);
        if (!bm.get())
        {
            return HC_ERROR_MAP_NOT_OPEN;
        }
        //通关才能进入下一图
        if (pc->m_tempo.isMapPassed(pc->m_cur_mapid))
        {
            ++pc->m_cur_mapid;
            pc->m_cur_stageid = 1;
            pc->m_tempo.InitCharTempo(pc->m_cur_mapid);
            pc->Save();
            //cout<<"auto move to next map..."<<endl;
        }
        else
        {
            //cout<<"!!! auto move to next map fail..."<<endl;
            return HC_ERROR_MAP_NOT_OPEN;
        }
    }

    json_spirit::Object info;
    boost::shared_ptr<CharStrongholdData> cd = Singleton<mapMgr>::Instance().GetCharStrongholdData(pc->m_id,mapid,stageid,strongholdpos);
    //如果目标关卡还不能打，先打他目前可以打的关卡
    if (!cd.get() || cd->m_state < 0)
    {
        cout<<"not open !!! cid:"<<pc->m_id<<",mapid"<<mapid<<"stage"<<stageid<<"pos"<<strongholdpos<<endl;
        cd = pc->m_tempo.getDestStronghold();
    }
    if (!cd.get())
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<baseStronghold> sd = cd->m_baseStronghold;
    if (!sd.get())
    {
        ERR();
        return HC_ERROR;
    }

    //关卡的信息
    info.push_back( Pair("mapid", mapid) );
    info.push_back( Pair("stageid", stageid) );
    info.push_back( Pair("pos", sd->m_pos));
    info.push_back( Pair("id", sd->m_id));
    info.push_back( Pair("name", sd->m_name));
    info.push_back( Pair("memo", sd->m_memo));
    info.push_back( Pair("tips", sd->m_tips));
    info.push_back( Pair("level", sd->m_level));
    info.push_back( Pair("spic", sd->m_spic));
    info.push_back( Pair("type", sd->m_type));
    info.push_back( Pair("need_times", sd->m_need_times));
    info.push_back( Pair("state", cd->m_state));
    info.push_back( Pair("cost_gold", sd->m_cost_gold));
    //俘虏关卡显示掉落信息
    if (sd->m_type == STRONGHOLD_TYPE_CAPTURE && sd->m_loot.get() && sd->m_Item.get())
    {
        info.push_back( Pair("get_info", *(sd->m_loot.get())));
    }
    //普通关卡和探索有奖励经验声望
    if (sd->m_type == STRONGHOLD_TYPE_NORMAL
        || sd->m_type == STRONGHOLD_TYPE_EXPLORE)
    {
        json_spirit::Array base_get;
        json_spirit::Object obj;
        //经验声望计算
        {
            obj.clear();
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_EXP, cd->exp_reward(), 0);
            itm.toObj(obj);
            base_get.push_back(obj);
        }
        for (int i = 1; i <= 4; ++i)
        {
            obj.clear();
            if (cd->prestige_reward(i) > 0)
            {
                Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN + i, cd->prestige_reward(i), 0);
                itm.toObj(obj);
                base_get.push_back(obj);
            }
        }
        info.push_back( Pair("base_get_info", base_get));
    }
    robj.push_back( Pair("stronghold_info", info));
    if (sd->m_hero.get())
    {
        info.clear();
        sd->m_hero->toObj(info);
        robj.push_back( Pair("hero_info", info) );
    }
    return HC_SUCCESS;
}

//获取关卡宝箱
int ProcessGetStrongholdBox(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int strongholdid = 0;
    READ_INT_FROM_MOBJ(strongholdid,o,"strongholdid");
    boost::shared_ptr<baseStronghold> shold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(strongholdid);
    if (!shold.get())
    {
        return HC_ERROR;
    }
    if (shold->m_type != STRONGHOLD_TYPE_BOX)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharStrongholdData> cd = Singleton<mapMgr>::Instance().GetCharStrongholdData(pc->m_id,shold->m_mapid,shold->m_stageid, shold->m_pos);
    if (cd.get())
    {
        //未开放或者已经拿过
        if (cd->m_state < 0 || cd->isPassed())
        {
            return HC_ERROR;
        }
        std::list<Item> getItems;
        Singleton<lootMgr>::Instance().getStrongholdLoots(strongholdid, getItems);
        if (!pc->m_bag.hasSlot(itemlistNeedBagSlot(getItems)))
        {
            return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
        }
        //关卡推进
        pc->m_tempo.update(strongholdid, true);
        //给东西
        giveLoots(pc, getItems, NULL, &robj, true, loot_stronghold);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//获取通关奖励
int ProcessGetStageFinishLoot(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    return pc->m_tempo.get_stage_finish_loot(mapid,stageid,robj);
}

//查询通关奖励
int ProcessCheckStageFinish(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int mapid = 0, stageid = 0;
    READ_INT_FROM_MOBJ(mapid, o, "mapid");
    READ_INT_FROM_MOBJ(stageid, o, "stageid");
    robj.push_back( Pair("mapid", mapid) );
    robj.push_back( Pair("stageid", stageid) );
    robj.push_back( Pair("can_get", pc->m_tempo.check_stage_finish(mapid,stageid)) );
    return HC_SUCCESS;
}

//点券通关
int ProcessGoldAttackStronghold(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    int strongholdid = 0, nums = 0;
    READ_INT_FROM_MOBJ(strongholdid, o, "strongholdid");
    READ_INT_FROM_MOBJ(nums, o, "nums");
    boost::shared_ptr<baseStronghold> shold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(strongholdid);
    if (!shold.get() || shold->m_type != STRONGHOLD_TYPE_NORMAL || shold->m_cost_gold < 0)
    {
        return HC_ERROR;
    }
    if (shold->m_type != STRONGHOLD_TYPE_NORMAL)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharStrongholdData> cd = Singleton<mapMgr>::Instance().GetCharStrongholdData(pc->m_id,shold->m_mapid,shold->m_stageid, shold->m_pos);
    if (cd.get())
    {
        //未开放
        if (cd->m_state < 0)
        {
            return HC_ERROR;
        }
    }
    else
    {
        return HC_ERROR;
    }
    if (nums <= 0)
        nums = 1;
    int cost_gold = nums * shold->m_cost_gold;
    if (cost_gold < 0)
    {
        return HC_ERROR;
    }
    if (pc->subGold(cost_gold,gold_cost_attack_stronghold) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    std::list<Item> items;
    //经验声望计算
    {
        Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_EXP, cd->exp_reward() * nums, 0);
        items.push_back(itm);
    }
    for (int i = 1; i <= 4; ++i)
    {
        if (cd->prestige_reward(i) > 0)
        {
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN + i, cd->prestige_reward(i) * nums, 0);
            items.push_back(itm);
        }
    }
    //给东西
    giveLoots(pc,items,NULL,&robj,true, loot_stronghold);
    return HC_SUCCESS;
}

void baseStrongholdHeroData::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("race", m_race));
    obj.push_back( Pair("name", m_name));
    obj.push_back( Pair("spic", m_spic));
    obj.push_back( Pair("star", m_star));
    obj.push_back( Pair("quality", m_quality));
    obj.push_back( Pair("level", m_level));
    obj.push_back( Pair("attack", m_attack));
    obj.push_back( Pair("defense", m_defense));
    obj.push_back( Pair("magic", m_magic));
    obj.push_back( Pair("hp", m_hp));
    obj.push_back( Pair("attack_power", m_attack + m_defense + m_magic) );
    return;
}

bool CharStrongholdData::isPassed()
{
    if (m_baseStronghold.get())
    {
        return m_state >= m_baseStronghold->m_need_times;
    }
    return false;
}

int CharStrongholdData::state()
{
    if (m_state < 1)
    {
        return m_state;
    }
    else if (isPassed())
    {
        return 1;
    }
    return 0;
}

int CharStrongholdData::exp_reward()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return 0;
    }
    if (m_baseStronghold.get() && m_baseStronghold->m_baseStage.get() && m_baseStronghold->m_baseStage->m_baseMap.get())
    {
        int base = 0;
        double extra_fac = 0.0;
        extra_fac += m_baseStronghold->m_baseStage->m_baseMap->m_exp_fac;
        //race_fac = 0;
        base = m_baseStronghold->m_base_exp;
        if (cdata->m_level_data.get())
        {
            extra_fac += cdata->m_level_data->m_reward_add;
        }
        double result = (double)base * (100.0 + extra_fac) / 100.0;
        return (int)result;
    }
    return 0;
}

int CharStrongholdData::prestige_reward(int race)
{
    if (race < 1)
        race = 1;
    if (race > 4)
        race = 4;
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return 0;
    }
    if (m_baseStronghold.get() && m_baseStronghold->m_baseStage.get() && m_baseStronghold->m_baseStage->m_baseMap.get())
    {
        int base = 0;
        double extra_fac = 0.0;
        double stage_fac = m_baseStronghold->m_baseStage->m_prestige_fac[race - 1];
        if (fabs(stage_fac - 0.0) < 1e-6)//场景加成配0则不奖励声望
            return 0;
        extra_fac += stage_fac;
        //race_fac = 0;
        base = m_baseStronghold->m_base_prestige;
        if (cdata->m_level_data.get())
        {
            extra_fac += cdata->m_level_data->m_reward_add;
        }
        double result = (double)base * (100.0 + extra_fac) / 100.0;
        return (int)result;
    }
    return 0;
}

void CharStageData::save()
{
    if (m_baseStage.get())
    {
        int size = m_baseStage->m_size;
        std::vector<int> m_stronghold_state;
        for (int i = 0; i < size; ++i)
        {
            if (m_strongholds[i].get())
            {
                m_stronghold_state.push_back(m_strongholds[i]->m_state);
            }
            else if (m_baseStage->m_baseStrongholds[i].get())
            {
                m_stronghold_state.push_back(-1);
                m_strongholds[i].reset(new CharStrongholdData(m_cid));
                CharStrongholdData* pshd = m_strongholds[i].get();
                pshd->m_baseStronghold = m_baseStage->m_baseStrongholds[i];
                pshd->m_state = -1;
            }
        }
        const json_spirit::Value val_state(m_stronghold_state.begin(), m_stronghold_state.end());
        InsertSaveDb("UPDATE char_stronghold SET stronghold_state='" + json_spirit::write(val_state)
                            + "' WHERE cid=" + LEX_CAST_STR(m_cid) + " AND stageid=" + LEX_CAST_STR(m_baseStage->m_id) + " AND mapid=" + LEX_CAST_STR(m_baseStage->m_mapid));
    }
}

int CharTempoData::load(int cid, int load_map = 0)
{
    //cout << "CharTempoData::load cid=" << cid << endl;
    Query q(GetDb());
    std::string sqlcmd = "SELECT mapid,stageid,stronghold_state FROM char_stronghold WHERE cid=" + LEX_CAST_STR(cid);
    if (load_map > 0)
    {
        sqlcmd += " and mapid=" + LEX_CAST_STR(load_map);
    }
    q.get_result(sqlcmd);
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        CharMapData *pMap = NULL;
        int mapid = q.getval();
        //是否存在该地图数据，没有则创建
        if (CharMapsData[mapid].get())
        {
            pMap = CharMapsData[mapid].get();
        }
        else
        {
            pMap = new CharMapData;
            CharMapsData[mapid].reset(pMap);
        }
        int stageid = q.getval();
        CharStageData *pStage = NULL;
        //是否存在该场景，没有则创建
        if ((*pMap)[stageid].get())
        {
            pStage = (*pMap)[stageid].get();
        }
        else
        {
            pStage = new CharStageData;
            boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(mapid);
            if (bm.get() && stageid >= 1 && stageid <= bm->m_baseStages.size())
            {
                pStage->m_baseStage = bm->m_baseStages[stageid-1];
            }
            (*pMap)[stageid].reset(pStage);
        }
        pStage->m_cid = cid;
        //解析各关卡状态
        std::string stronghold_state = q.getstr();
        std::vector<int> m_stronghold_state;
        boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(mapid);
        if(stronghold_state == "" && bm.get() && bm->m_baseStages[stageid-1]->m_size)
        {
            m_stronghold_state.clear();
            m_stronghold_state.insert(m_stronghold_state.begin(), bm->m_baseStages[stageid-1]->m_size, -1);
            m_stronghold_state[0] = 0;
        }
        json_spirit::Value types;
        json_spirit::read(stronghold_state, types);
        if (types.type() == json_spirit::array_type)
        {
            json_spirit::Array& types_array = types.get_array();
            for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
            {
                if ((*it).type() != json_spirit::int_type)
                {
                    break;
                }
                m_stronghold_state.push_back((*it).get_int());
            }
            while (bm.get() && m_stronghold_state.size() < bm->m_baseStages[stageid-1]->m_size)
            {
                //cout << m_stronghold_state.size() << "," << bm->m_baseStages[stageid-1]->m_size << endl;
                m_stronghold_state.push_back(-1);
            }
        }
        else
        {
            ERR();
        }
        //各关卡状态初始化到玩家进度
        int pos = 1;
        for (std::vector<int>::iterator it_i = m_stronghold_state.begin(); it_i < m_stronghold_state.end(); ++it_i)
        {
            int m_state = *it_i;
            boost::shared_ptr<baseStronghold> bstronghold = Singleton<mapMgr>::Instance().GetBaseStrongholdData(mapid, stageid, pos);
            if (!bstronghold.get())
            {
                ERR();
                break;
            }
            boost::shared_ptr<CharStrongholdData> p_tmp;
            p_tmp.reset(new CharStrongholdData(cid));
            if (pos == pStage->m_strongholds.size() + 1)
            {
                pStage->m_strongholds.push_back(p_tmp);
            }
            else
            {
                ERR();
            }

            /******v1.1调整关卡攻击次数对老帐号进度处理***/
            //下个关卡状态已经可攻击
            if (pos < m_stronghold_state.size())
            {
                if (m_stronghold_state[pos] >= 0 && m_state < bstronghold->m_need_times)
                {
                    m_state = bstronghold->m_need_times;
                }
            }
            /**********************/

            p_tmp->m_baseStronghold = bstronghold;
            p_tmp->m_state = m_state;
            //cout << "pos=[" << mapid << "," << stageid << "," << pos << "] id=" << bstronghold->m_id << " cur " << m_state << ",need=" << bstronghold->m_need_times << endl;
            if (m_state >= bstronghold->m_need_times)
            {
                if (m_charData.m_cur_strongholdid < bstronghold->m_id)
                {
                    m_charData.m_cur_strongholdid = bstronghold->m_id;
                    //cout << "set cur_stronghold=" << m_charData.m_cur_strongholdid << endl;
                }
                m_charData.m_cur_stageid = bstronghold->m_stageid;
                //cout << "pass!!!!!! set cur_stageid=" << bstronghold->m_stageid << endl;
            }
            else if (m_state >= 0)
            {
                m_charData.m_cur_stageid = bstronghold->m_stageid;
                //cout << "state > 0 set cur_stageid=" << bstronghold->m_stageid << endl;
            }
            ++pos;
        }
    }
    q.free_result();
    int tmp = m_charData.queryExtraData(char_data_type_normal, char_data_normal_stronghold);
    if (tmp != m_charData.m_cur_strongholdid)
    {
        boost::shared_ptr<baseStronghold> bs = Singleton<mapMgr>::Instance().GetBaseStrongholdData(m_charData.m_cur_strongholdid);
        if (bs.get() && bs->m_type != STRONGHOLD_TYPE_BOX)
        {
            m_charData.setExtraData(char_data_type_normal, char_data_normal_stronghold, m_charData.m_cur_strongholdid);
        }
    }
    return 0;
}

int CharTempoData::update(int stronghold, bool bBroad = true)
{
    boost::shared_ptr<baseStronghold> stronghold_data = Singleton<mapMgr>::Instance().GetBaseStrongholdData(stronghold);
    baseStronghold *bShold = stronghold_data.get();
    if (NULL == bShold)
    {
        ERR();
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(bShold->m_mapid);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            ERR();
            return HC_ERROR;
        }
        CharMapData::iterator itm = md->find(bShold->m_stageid);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                ERR();
                cout<<m_charData.m_id<<"CharStageData NULL,strongholdid,map,stage,"<<stronghold<<","<<bShold->m_mapid<<","<<bShold->m_stageid<<endl;
                return HC_ERROR;
            }
            CharStrongholdData* pCShold = cStage->m_strongholds[bShold->m_pos - 1].get();
            if (pCShold == NULL)
            {
                ERR();
                cout<<m_charData.m_id<<"CharStrongholdData NULL,strongholdid,map,stage,"<<stronghold<<","<<bShold->m_mapid<<","<<bShold->m_stageid<<endl;
                return HC_ERROR;
            }
            //设置总攻击次数
            pCShold->m_state++;

            //首次占领处理
            if (pCShold->m_state == bShold->m_need_times)
            {
                if (bShold->m_guide_id > 0)
                {
                    //新手引导
                    m_charData.checkGuide(bShold->m_guide_id);
                }
                if (m_charData.m_cur_strongholdid < stronghold)
                {
                    m_charData.m_cur_strongholdid = stronghold;
                    int tmp = m_charData.queryExtraData(char_data_type_normal, char_data_normal_stronghold);
                    if (tmp != m_charData.m_cur_strongholdid)
                    {
                        if (stronghold_data->m_type != STRONGHOLD_TYPE_BOX)
                        {
                            m_charData.setExtraData(char_data_type_normal, char_data_normal_stronghold, m_charData.m_cur_strongholdid);
                        }
                    }
                    //功能开放
                    //m_charData.updateOpen();
                    //任务接取
                }
                //有些关卡第一次打败后送东西
                switch (stronghold)
                {
                    default:
                        break;
                }
                //更新任务
                m_charData.m_tasks.updateTask(GOAL_STRONGHOLD, stronghold, 1);
                //活动更新
                if (Singleton<actionMgr>::Instance().isStrongholdActionOpen(&m_charData))
                {
                    Singleton<actionMgr>::Instance().updateStrongholdAction(m_charData.m_id, stronghold);
                }
                //周排行活动
                int score = bShold->m_mapid * 50 + bShold->m_stageid * 10 + bShold->m_pos;
                if (score > 0)
                    Singleton<weekRankings>::Instance().updateEventRankings(m_charData.m_id,week_ranking_stronghold,score);
            }
            //开放下个关卡
            if (pCShold->m_state >= bShold->m_need_times)
            {
                if (bShold->m_pos < cStage->m_baseStage->m_size && cStage->m_strongholds[bShold->m_pos].get())
                {
                    if (cStage->m_strongholds[bShold->m_pos]->m_state < 0)
                    {
                        cStage->m_strongholds[bShold->m_pos]->m_state = 0;
                    }
                }
                else if(bShold->m_pos == cStage->m_baseStage->m_size)//下个场景
                {
                    //看是否要开放下一个场景
                    if (++itm != (*md).end())
                    {
                        CharStageData* pNextStage = itm->second.get();
                        if (pNextStage == NULL)
                        {
                            ERR();
                            return HC_ERROR;
                        }
                        if (!pNextStage->m_baseStage.get())
                        {
                            ERR();
                            return HC_ERROR;
                        }
                        if (pNextStage->m_baseStage->m_size <= 0)
                        {
                            ERR();
                            return HC_ERROR;
                        }
                        if (!pNextStage->m_strongholds[0].get())
                        {
                            ERR();
                            return HC_ERROR;
                        }
                        //下个场景没开开放下一个场景
                        if (pNextStage->m_strongholds[0]->m_state < 0)
                        {
                            pNextStage->m_strongholds[0]->m_state = 0;
                            m_charData.m_cur_stageid = pNextStage->m_baseStage->m_id;
                        }
                        pNextStage->save();
                    }
                    else//下个地图
                    {
                        //通关才能进入下一图
                        if (isMapPassed(m_charData.m_cur_mapid))
                        {
                            ++m_charData.m_cur_mapid;
                            InitCharTempo(m_charData.m_cur_mapid);
                            m_charData.Save();
                            //更新任务
                            m_charData.m_tasks.updateTask(GOAL_MAP, m_charData.m_cur_mapid, 1);
                            if (bBroad)
                            {
                                /*********** 广播 ************/
                                std::string msg = strSystemPassMap;
                                boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(m_charData.m_cur_mapid);
                                if (bm.get())
                                {
                                    msg = strSystemPassMap2;
                                    str_replace(msg, "$N", bm->m_name);
                                }
                                str_replace(msg, "$W", MakeCharNameLink(m_charData.m_name,m_charData.m_nick.get_string()));
                                str_replace(msg, "$T", bShold->m_baseStage->m_baseMap->m_name);
                                GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                            }
                            //广播好友祝贺
                            Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_PASS_MAP, m_charData.m_cur_mapid, 0);
                        }
                    }
                }
            }
            //当前场景进度更新后保存
            cStage->save();
        }
        else
        {
            ERR();
        }
    }
    return HC_SUCCESS;
}

//插入角色关卡进度
int CharTempoData::InitCharTempo(int mapid)
{
    //插入玩家关卡进度
    boost::shared_ptr<baseMap> bm = Singleton<mapMgr>::Instance().GetBaseMap(mapid);
    if (!bm.get())
    {
        ERR();
        return HC_ERROR;
    }
    Query q(GetDb());
    for (int stageid = 1; stageid <= bm->m_baseStages.size(); ++stageid)
    {
        if (bm->m_baseStages[stageid-1].get())
        {
            int size = bm->m_baseStages[stageid-1]->m_size;
            std::vector<int> m_stronghold_state;
            for (int i = 0; i < size; ++i)
            {
                if (bm->m_baseStages[stageid-1]->m_baseStrongholds[i].get())
                {
                    if (stageid == 1 && bm->m_baseStages[stageid-1]->m_baseStrongholds[i]->m_pos == 1)
                    {
                        m_stronghold_state.push_back(0);
                    }
                    else
                    {
                        m_stronghold_state.push_back(-1);
                    }
                }
            }
            const json_spirit::Value val_state(m_stronghold_state.begin(), m_stronghold_state.end());
            if (!q.execute("replace into char_stronghold set cid=" + LEX_CAST_STR(m_charData.m_id) + ",mapid=" + LEX_CAST_STR(mapid) + ",stageid=" + LEX_CAST_STR(stageid) + ",stronghold_state='"+json_spirit::write(val_state)+"'"))
            {
                DB_ERROR(q);
            }
        }
    }
    //加载
    load(m_charData.m_id, mapid);
    return HC_SUCCESS;
}

int CharTempoData::Save()
{
    Query q(GetDb());
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.begin();
    while (it != CharMapsData.end())
    {
        CharMapData::iterator it1 = it->second->begin();
        while (it1 != it->second->end())
        {
            boost::shared_ptr<CharStageData> psd = it1->second;
            if (psd.get() && psd->m_baseStage.get())
            {
                psd->save();
            }
            ++it1;
        }
        ++it;
    }
    return 0;
}

//某张地图是否已经通关
bool CharTempoData::isMapPassed(int mapid)
{
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(mapid);
    if (it != CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).begin();
        while (itm != (*md).end())
        {
            if (itm->second.get() && itm->second->m_baseStage.get())
            {
                if (itm->second->m_strongholds[0].get() && itm->second->m_strongholds[itm->second->m_baseStage->m_size - 1].get())
                {
                    if (itm->second->m_strongholds[itm->second->m_baseStage->m_size - 1]->isPassed())
                    {
                        ;
                    }
                    else
                    {
                        return false;
                    }
                }
                else
                {
                    return false;
                }
            }
            ++itm;
        }
        return true;
    }
    else
    {
        return false;
    }
}

boost::shared_ptr<CharStrongholdData> CharTempoData::getDestStronghold()
{
    boost::shared_ptr<CharStrongholdData> cd;
    boost::shared_ptr<baseStronghold> next = Singleton<mapMgr>::Instance().GetBaseStrongholdData(m_charData.m_cur_strongholdid+1);
    if (next.get())
    {
        cd = Singleton<mapMgr>::Instance().GetCharStrongholdData(m_charData.m_id, next->m_mapid, next->m_stageid, next->m_pos);
        return cd;
    }
    else
    {
        boost::shared_ptr<baseStronghold> cur = Singleton<mapMgr>::Instance().GetBaseStrongholdData(m_charData.m_cur_strongholdid);
        if (cur.get())
        {
            cd = Singleton<mapMgr>::Instance().GetCharStrongholdData(m_charData.m_id, cur->m_mapid, cur->m_stageid, cur->m_pos);
            return cd;
        }
    }
    cd.reset();
    return cd;
}

int CharTempoData::get_stage_finish_loot(int mapid, int stageid , json_spirit::Object& robj)
{
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(mapid);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            ERR();
            return HC_ERROR;
        }
        CharMapData::iterator itm = md->find(stageid);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                ERR();
                cout<<m_charData.m_id<<"CharStageData NULL"<<endl;
                return HC_ERROR;
            }
            //场景最后关卡
            CharStrongholdData* pCShold = cStage->m_strongholds[cStage->m_baseStage->m_size - 1].get();
            if (pCShold == NULL)
            {
                ERR();
                cout<<m_charData.m_id<<"CharStrongholdData NULL"<<endl;
                return HC_ERROR;
            }
            //攻打成功
            if (pCShold->isPassed())
            {
                //是否已经领取过了
                int idx = char_data_normal_stage_reward_start + stageIndex(mapid,stageid);
                int get = m_charData.queryExtraData(char_data_type_normal, idx);
                if (get)
                {
                    return HC_ERROR;
                }
                //设置已经领取
                m_charData.setExtraData(char_data_type_normal, idx, 1);

                // 通关奖励
                std::list<Item> items;
                int id = stageIndex(mapid,stageid);
                //cout << "get_stage_finish_loot !!! mapid=" << mapid << ",stageid=" << stageid << endl;
                Singleton<lootMgr>::Instance().getStageLoots(id, items, robj);
                //给东西
                giveLoots(&m_charData, items, NULL, NULL, true, loot_stage);
                return HC_SUCCESS;
            }
        }
        else
        {
            ERR();
        }
    }
    return HC_ERROR;
}

bool CharTempoData::check_stage_finish(int mapid, int stageid)
{
    //是否已经领取过了
    int idx = char_data_normal_stage_reward_start + stageIndex(mapid,stageid);
    int get = m_charData.queryExtraData(char_data_type_normal, idx);
    if (get)
    {
        return false;
    }
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = CharMapsData.find(mapid);
    if (it != CharMapsData.end())
    {
        //所在的地图数据
        CharMapData* md = it->second.get();
        if (md == NULL)
        {
            ERR();
            return false;
        }
        CharMapData::iterator itm = md->find(stageid);
        if (itm != md->end() && itm->second.get() && itm->second->m_baseStage.get())
        {
            //所在场景
            CharStageData* cStage = itm->second.get();
            if (NULL == cStage)
            {
                ERR();
                cout<<m_charData.m_id<<"CharStageData NULL"<<mapid<<","<<stageid<<endl;
                return false;
            }
            //场景最后关卡
            CharStrongholdData* pCShold = cStage->m_strongholds[cStage->m_baseStage->m_size - 1].get();
            if (pCShold == NULL)
            {
                ERR();
                cout<<m_charData.m_id<<"CharStrongholdData NULL"<<mapid<<","<<stageid<<endl;
                return false;
            }
            //第一次攻打成功
            if (pCShold->isPassed())
            {
                return true;
            }
        }
        else
        {
            ERR();
        }
    }
    return false;
}

mapMgr::mapMgr()
{
    cout<<"************ mapMgr::reloadMap ******************"<<endl;

    Query q(GetDb());
    q.get_result("select id,name,memo,exp_fac from base_maps where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        if (mapid == m_base_maps.size() + 1)
        {
            boost::shared_ptr<baseMap> bm;
            bm.reset(new baseMap);
            m_base_maps.push_back(bm);
            bm->m_id = mapid;
            bm->m_name = q.getstr();
            bm->m_memo = q.getstr();
            bm->m_exp_fac = q.getnum();
        }
        else
        {
            ERR();
        }
    }
    q.free_result();

    cout<<"************ mapMgr::reloadStage ******************"<<endl;

    q.get_result("select mapid,id,spic,name,minlevel,maxlevel,prestige1_fac,prestige2_fac,prestige3_fac,prestige4_fac from base_stages where 1 order by mapid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        int stageid = q.getval();
        if (!m_base_maps[mapid-1].get())
        {
            ERR();
            continue;
        }
        if (stageid == m_base_maps[mapid-1]->m_baseStages.size() + 1)
        {
            boost::shared_ptr<baseStage> bs;
            bs.reset(new baseStage);
            m_base_maps[mapid-1]->m_baseStages.push_back(bs);
            bs->m_id = stageid;
            bs->m_mapid = mapid;
            bs->m_spic = q.getval();
            bs->m_name = q.getstr();
            bs->m_minlevel = q.getval();
            bs->m_maxlevel = q.getval();
            bs->m_size = 0;
            for (int i = 0; i < 4; ++i)
            {
                bs->m_prestige_fac[i] = q.getnum();
            }
            bs->m_baseMap.reset();
            bs->m_baseMap = m_base_maps[mapid-1];
        }
        else
        {
            ERR();
        }
    }
    q.free_result();

    cout<<"************ mapMgr::reloadStronghold ******************"<<endl;

    q.get_result("SELECT id,level,mapid,stageid,strongholdpos,type,name,memo,tips,chat,spic,x,y,need_times,cost_gold,silver,base_exp,base_prestige,guide_id,race,star,attack,defense,magic,hp,skill,card_rank,path FROM base_stronghold WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int strongholdid = q.getval();
        boost::shared_ptr<baseStronghold> p_data = m_base_stronghold_map[strongholdid];
        if (!p_data.get())
        {
            p_data.reset(new baseStronghold);
            m_base_stronghold_map[strongholdid] = p_data;
        }
        p_data->m_id = strongholdid;
        p_data->m_level = q.getval();
        p_data->m_mapid = q.getval();
        p_data->m_stageid = q.getval();
        p_data->m_pos = q.getval();
        p_data->m_type = q.getval();
        p_data->m_name = q.getstr();
        p_data->m_memo = q.getstr();
        p_data->m_tips = q.getstr();
        p_data->m_chat = q.getstr();
        p_data->m_spic = q.getval();
        p_data->m_station.m_x = q.getval();
        p_data->m_station.m_y = q.getval();
        p_data->m_need_times = q.getval();
        p_data->m_cost_gold = q.getval();
        p_data->m_silver = q.getval();
        p_data->m_base_exp = q.getval();
        p_data->m_base_prestige = q.getval();
        p_data->m_guide_id = q.getval();
        //英雄信息
        boost::shared_ptr<baseStrongholdHeroData> sh;
        if (!(p_data->m_hero.get()))
        {
            sh.reset(new (baseStrongholdHeroData));
            p_data->m_hero = sh;
        }
        else
        {
            sh = p_data->m_hero;
        }
        sh->m_name = p_data->m_name;
        sh->m_spic = p_data->m_spic;
        sh->m_race = q.getval();
        sh->m_star = q.getval();
        sh->m_quality = sh->m_star;
        sh->m_level = p_data->m_level;
        sh->m_attack = q.getval();
        sh->m_defense = q.getval();
        sh->m_magic = q.getval();
        sh->m_hp = q.getval();
        std::string skill_data = q.getstr();
        if(skill_data == "")
        {
            ;
        }
        else
        {
            json_spirit::mValue types;
            json_spirit::read(skill_data, types);
            if (types.type() == json_spirit::array_type)
            {
                json_spirit::mArray& types_array = types.get_array();
                for (json_spirit::mArray::iterator it = types_array.begin(); it != types_array.end(); ++it)
                {
                    if ((*it).type() != json_spirit::obj_type)
                    {
                        ERR();
                        break;
                    }
                    int skill_id = 0, skill_level = 0;
                    json_spirit::mObject& o = (*it).get_obj();
                    READ_INT_FROM_MOBJ(skill_id,o,"id");
                    READ_INT_FROM_MOBJ(skill_level,o,"level");
                }
            }
        }
        //解析关卡牌型
        std::string stronghold_card = q.getstr();
        if(stronghold_card == "")
        {
            p_data->m_card_rank.clear();
        }
        else
        {
            json_spirit::Value types;
            json_spirit::read(stronghold_card, types);
            if (types.type() == json_spirit::array_type)
            {
                json_spirit::Array& types_array = types.get_array();
                for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
                {
                    if ((*it).type() != json_spirit::int_type)
                    {
                        break;
                    }
                    p_data->m_card_rank.push_back((*it).get_int());
                }
            }
        }
        //解析关卡路径
        std::string stronghold_path = q.getstr();
        if(stronghold_path == "")
        {
            p_data->m_path.clear();
        }
        else
        {
            json_spirit::Value paths;
            json_spirit::read(stronghold_path, paths);
            if (paths.type() == json_spirit::array_type)
            {
                json_spirit::Array& paths_array = paths.get_array();
                for (json_spirit::Array::iterator it = paths_array.begin(); it != paths_array.end(); ++it)
                {
                    if ((*it).type() != json_spirit::array_type)
                    {
                        ERR();
                        break;
                    }
                    json_spirit::Array& pos_array = (*it).get_array();
                    int x = -1, y = -1;
                    bool xGet = false;
                    for (json_spirit::Array::iterator it_p = pos_array.begin(); it_p != pos_array.end(); ++it_p)
                    {
                        if ((*it_p).type() != json_spirit::int_type)
                        {
                            ERR();
                            break;
                        }
                        if (!xGet)
                        {
                            x = (*it_p).get_int();
                            xGet = true;
                        }
                        else
                        {
                            y = (*it_p).get_int();
                        }
                    }
                    if (x != -1 && y != -1)
                    {
                        p_data->m_path.push_back(Pos(x,y));
                    }
                }
            }
        }
        p_data->m_Item = Singleton<lootMgr>::Instance().getStrongholdLootInfo(strongholdid);
        if (p_data->m_Item.get())
            p_data->m_Item->toPointObj(p_data->m_loot);

        boost::shared_ptr<baseMap> bm = m_base_maps[p_data->m_mapid-1];
        if (bm.get() && (p_data->m_stageid >= 1 && p_data->m_stageid <= bm->m_baseStages.size()) && bm->m_baseStages[p_data->m_stageid-1].get())
        {
            if (p_data->m_pos == bm->m_baseStages[p_data->m_stageid-1]->m_baseStrongholds.size() + 1)
            {
                p_data->m_baseStage = bm->m_baseStages[p_data->m_stageid-1];
                bm->m_baseStages[p_data->m_stageid-1]->m_baseStrongholds.push_back(p_data);
                bm->m_baseStages[p_data->m_stageid -1]->m_size++;
            }
            else
            {
                ERR();
            }
        }
        else
        {
            ERR();
        }
    }
    q.free_result();
}

//获得关卡id，参数地图id，场景id，关卡编号
int mapMgr::GetStrongholdid(int mapid, int stageid, int pos)
{
    boost::shared_ptr<baseStronghold> bs = GetBaseStrongholdData(mapid,stageid,pos);
    if (bs.get())
    {
        return bs->m_id;
    }
    return 0;
}

//获得关卡位置信息
int mapMgr::GetStrongholdPos(int& mapid, int& stageid, int strongholdid)
{
    boost::shared_ptr<baseStronghold> bs = GetBaseStrongholdData(strongholdid);
    if (bs.get())
    {
        mapid = bs->m_mapid;
        stageid = bs->m_stageid;
        return bs->m_pos;
    }
    return 0;
}

//获得地图描述信息
int mapMgr::GetMapMemo(int mapid, std::string& name, std::string& memo)
{
    boost::shared_ptr<baseMap> bm = GetBaseMap(mapid);
    if (bm.get())
    {
        name = bm->m_name;
        memo = bm->m_memo;
        return 0;
    }
    else
    {
        ERR();
        return -1;
    }
}

//基础关卡信息
boost::shared_ptr<baseStronghold> mapMgr::GetBaseStrongholdData(int strongholdid)
{
    std::map<int, boost::shared_ptr<baseStronghold> >::iterator it = m_base_stronghold_map.find(strongholdid);
    if (it != m_base_stronghold_map.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseStronghold> gd;
        gd.reset();
        return gd;
    }
}

//基础关卡信息
boost::shared_ptr<baseStronghold> mapMgr::GetBaseStrongholdData(int mapid, int stageid, int pos)
{
    if (mapid >= 1 && mapid <= m_base_maps.size() && m_base_maps[mapid-1].get() && stageid >= 1
        && stageid <=m_base_maps[mapid-1]->m_baseStages.size() && m_base_maps[mapid-1]->m_baseStages[stageid-1].get()
        && pos >= 1 && pos <= m_base_maps[mapid-1]->m_baseStages[stageid-1]->m_baseStrongholds.size())
    {
        return m_base_maps[mapid-1]->m_baseStages[stageid-1]->m_baseStrongholds[pos-1];
    }
    else
    {
        boost::shared_ptr<baseStronghold> gd;
        gd.reset();
        return gd;
    }
}

//基础地图
boost::shared_ptr<baseMap> mapMgr::GetBaseMap(int mapid)
{
    if (mapid >= 1 && mapid <= 10)
    {
        return m_base_maps[mapid-1];
    }
    else
    {
        boost::shared_ptr<baseMap> bm;
        return bm;
    }
}

boost::shared_ptr<CharStrongholdData> mapMgr::GetCharStrongholdData(int cid, int mapid, int stageid, int strongholdpos)
{
    //获得角色信息
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    std::map<int, boost::shared_ptr<CharMapData> >::iterator it = cdata->m_tempo.CharMapsData.find(mapid);
    if (it != cdata->m_tempo.CharMapsData.end())
    {
        boost::shared_ptr<CharMapData> md = it->second;
        CharMapData::iterator itm = (*md).find(stageid);
        if (itm != (*md).end() && strongholdpos <= itm->second->m_strongholds.size())
        {
            return itm->second->m_strongholds[strongholdpos - 1];
        }
    }
    boost::shared_ptr<CharStrongholdData> p_sd;
    p_sd.reset();
    return p_sd;
}

int mapMgr::combatResult(chessCombat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_STRONGHOLD != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_players[0].m_cid);
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<baseStronghold> bs = GetBaseStrongholdData(pCombat->m_data_id);
    if (!bs.get())
    {
        ERR();
        return HC_ERROR;
    }
    CharStrongholdData* pcStronghold = GetCharStrongholdData(pCombat->m_players[0].m_cid, bs->m_mapid, bs->m_stageid, bs->m_pos).get();
    if (!pcStronghold || !pcStronghold->m_baseStronghold.get())
    {
        ERR();
        return HC_ERROR;
    }
    cout << "mapMgr::combatResult   " << pCombat->m_combat_id << ",reuslt=" << pCombat->m_result << endl;
    //探索已经通关的地方
    if (pcStronghold->isPassed() && pcStronghold->m_baseStronghold->m_type == STRONGHOLD_TYPE_EXPLORE)
    {
        //无论输赢都设置今日完成
        cdata->setExtraData(char_data_type_daily,char_data_daily_explore_begin+pcStronghold->m_baseStronghold->m_id,1);
    }
    cdata->m_score_tasks.updateTask(DAILY_SCORE_STRONGHOLD);
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        bool has_reward = true;
        //通关过的关卡
        if (pcStronghold->isPassed())
        {
            //获胜更新声望任务
            cdata->m_prestige_tasks.update(pCombat->m_data_id);
            //战力高于对手无收益
            int a_power = pCombat->m_players[0].m_attack + pCombat->m_players[0].m_defense + pCombat->m_players[0].m_magic;
            int b_power = pCombat->m_players[1].m_attack + pCombat->m_players[1].m_defense + pCombat->m_players[1].m_magic;
            if (a_power > b_power)
            {
                has_reward = false;
            }
        }
        //关卡推进
        cdata->m_tempo.update(pCombat->m_data_id, true);
        //普通关卡和探索有奖励经验声望
        if (pcStronghold->m_baseStronghold->m_type == STRONGHOLD_TYPE_NORMAL
            || pcStronghold->m_baseStronghold->m_type == STRONGHOLD_TYPE_EXPLORE)
        {
            //关卡掉落
            int limit_times = 0, cur_times = 0, item_type = 0, item_id = 0;
            do
            {
                limit_times = 0;
                cur_times = 0;
                item_type = 0;
                item_id = 0;
                if (Singleton<lootMgr>::Instance().getStrongholdLoots(pCombat->m_data_id, pCombat->m_getItems) > 0)
                {
                    std::list<Item>::iterator it_l = pCombat->m_getItems.begin();
                    if (it_l != pCombat->m_getItems.end())
                    {
                        item_type = it_l->type;
                        item_id = it_l->id;
                        limit_times = Singleton<lootMgr>::Instance().getLootLimit(item_type, item_id);
                        cur_times = cdata->getLootTimes(item_type, item_id);
                    }
                }
            }while(limit_times > 0 && cur_times > limit_times);
            if (limit_times > 0 && cur_times > 0)
            {
                cdata->setLootTimes(item_type, item_id, cur_times + 1);
            }
            //经验声望计算
            if (has_reward)
            {
                Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_EXP, pcStronghold->exp_reward() * pCombat->m_extra_data[0], 0);
                pCombat->m_getItems.push_back(itm);
                for (int i = 1; i <= 4; ++i)
                {
                    Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN + i, pcStronghold->prestige_reward(i) * pCombat->m_extra_data[0], 0);
                    pCombat->m_getItems.push_back(itm);
                }
            }
            if (pCombat->m_getItems.size())
            {
                giveLoots(cdata, pCombat, true, loot_stronghold);
            }
        }
        else if(pcStronghold->m_baseStronghold->m_type == STRONGHOLD_TYPE_CAPTURE)
        {
            //俘虏英雄掉落需要玩家没有英雄
            if (pcStronghold->m_baseStronghold->m_Item.get())
            {
                if (pcStronghold->m_baseStronghold->m_Item->type == ITEM_TYPE_HERO)
                {
                    if (cdata->m_heros.GetHeroByType(pcStronghold->m_baseStronghold->m_Item->id) == 0)
                    {
                        //通关则必掉
                        if (pcStronghold->isPassed())
                        {
                            pCombat->m_getItems.push_back(*(pcStronghold->m_baseStronghold->m_Item.get()));
                        }
                        else
                        {
                            Singleton<lootMgr>::Instance().getStrongholdLoots(pCombat->m_data_id, pCombat->m_getItems);
                        }
                    }
                }
            }
            if (pCombat->m_getItems.size())
            {
                giveLoots(cdata, pCombat, true, loot_stronghold);
            }
        }
        cdata->m_tasks.updateTask(GOAL_STRONGHOLD_ATTACK, pcStronghold->m_baseStronghold->m_id, 1);
        cdata->m_tasks.updateTask(GOAL_DAILY_STRONGHOLD, pcStronghold->m_baseStronghold->m_id, 1);
    }
    else if (pCombat->m_result == COMBAT_RESULT_ATTACK_LOSE)
    {
        for (int i = 1; i <= 4; ++i)
        {
            Item itm(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN + i, 0, 0);
            pCombat->m_getItems.push_back(itm);
        }
        //给东西
        giveLoots(cdata, pCombat, true, loot_stronghold);
    }
    return HC_SUCCESS;
}

