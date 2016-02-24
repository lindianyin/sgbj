
#include "char_zst.h"
#include <algorithm>    // std::random_shuffle
#include "singleton.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "statistics.h"

#include "combat.h"
#include "igeneral.h"
#include "loot.h"
#include "SaveDb.h"
#include "char_general_soul.hpp"

Database& GetDb();
int giveLoots(CharData* cdata, Item& getItem, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);

extern Combat* createZSTCombat(int cid, int zst_army_id, int extra_data, int& ret, bool);
extern void InsertCombat(Combat* pcombat);
extern void InsertSaveCombat(Combat* pcombat);
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

const int iZSTFreeRefreshTimes = 2;
const int iZSTRefreshCost = 5;
const int iZSTRefresh5Cost = 100;

const int iZSTBuyChallengeCost = 100;
const int iZSTChallengeTimes = 3;

inline int getZstBuyCost(int times)
{
    int t = times;
    int costs_more[] = {1, 1, 2, 2, 4, 4, 8};
    int ssize = sizeof(costs_more)/sizeof(int);
    if (t < 0)
    {
        t = 0;
    }
    else if (t >= ssize)
    {
        t = ssize - 1;
    }
    return iZSTBuyChallengeCost*costs_more[t];
}

void CharZSTStageData::load()
{
    Query q(GetDb());
    //进度数据
    q.get_result("select state,result_star,stronghold_state,stronghold_star from char_zst_maps where cid=" + LEX_CAST_STR(m_cid) + 
            + " and mapid=" + LEX_CAST_STR(m_mapid)
            + " and stageid=" + LEX_CAST_STR(m_stageid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_state = q.getval();
        m_result_star = q.getval();
        std::string stronghold_state = q.getstr();
        std::string stronghold_star = q.getstr();

        if(stronghold_state == "")
        {
            m_stronghold_state.clear();
            m_stronghold_state.insert(m_stronghold_state.begin(), 5, 0);
        }
        else
        {
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
                while (m_stronghold_state.size() < 5)
                {
                    m_stronghold_state.push_back(0);
                }
            }
            else
            {
                ERR();
            }
        }

        if(stronghold_star == "")
        {
            m_stronghold_star.clear();
            m_stronghold_star.insert(m_stronghold_star.begin(), 5, 1);
        }
        else
        {
            json_spirit::Value types;
            json_spirit::read(stronghold_star, types);
            if (types.type() == json_spirit::array_type)
            {
                json_spirit::Array& types_array = types.get_array();
                for (json_spirit::Array::iterator it = types_array.begin(); it != types_array.end(); ++it)
                {
                    if ((*it).type() != json_spirit::int_type)
                    {
                        break;
                    }
                    m_stronghold_star.push_back((*it).get_int());
                }
                while (m_stronghold_star.size() < 5)
                {
                    m_stronghold_star.push_back(1);
                }
            }
            else
            {
                ERR();
            }
        }
    }
    q.free_result();
    m_baseStage = Singleton<zstMgr>::Instance().getBaseZSTStage(m_mapid,m_stageid);
    return;
}

void CharZSTStageData::Save()
{
    if (m_baseStage.get())
    {
        const json_spirit::Value val_state(m_stronghold_state.begin(), m_stronghold_state.end());
        const json_spirit::Value val_star(m_stronghold_star.begin(), m_stronghold_star.end());
        InsertSaveDb("replace into char_zst_maps (cid,mapid,stageid,state,result_star,stronghold_state,stronghold_star) values ("
            + LEX_CAST_STR(m_cid) + ","
            + LEX_CAST_STR(m_mapid) + ","
            + LEX_CAST_STR(m_stageid) + ","
            + LEX_CAST_STR(m_state) + ","
            + LEX_CAST_STR(m_result_star) + ",'"
            + json_spirit::write(val_state) + "','"
            + json_spirit::write(val_star) + "')");
    }
}

int CharZSTStageData::getStar()
{
    int tmp = 0;
    for(std::vector<int>::iterator it = m_stronghold_star.begin(); it != m_stronghold_star.end(); ++it)
    {
        tmp += *it;
    }
    return tmp / m_stronghold_star.size();
}

void char_zst_general::Save()
{
    InsertSaveDb("update char_zst_generals set hurt_hp=" + LEX_CAST_STR(m_hp_hurt)
            + ",max_hp=" + LEX_CAST_STR(m_hp_org)
            + ",org_hurt_hp=" + LEX_CAST_STR(m_org_hp_hurt)
            + " where cid=" + LEX_CAST_STR(cid) + " and guid=" + LEX_CAST_STR(id)
        );
}

void char_zst::load()
{
    Query q(GetDb());
    q.get_result("select mapid,stageid,star_reward,star,star_update_time from char_zst where cid=" + LEX_CAST_STR(m_cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_cur_map = q.getval();
        m_cur_stage = q.getval();
        m_cur_star_reward = q.getval();
        m_total_star = q.getval();
        m_star_update_time = q.getval();

        q.free_result();
        
        if (m_cur_map > 0 && m_cur_stage > 0)
        {
            //武将数据
            q.get_result("select pos,guid,gid,level,color,nickname,str,wisdom,tong,org_attack,org_wufang,org_cefang,org_max_hp,org_hurt_hp,combatAttr,max_hp,hurt_hp,inspired from char_zst_generals where cid=" + LEX_CAST_STR(m_cid) + " order by pos");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                char_zst_general cg;
                cg.pos = q.getval();
                cg.id = q.getval();
                cg.gid = q.getval();
                cg.level = q.getval();

                boost::shared_ptr<GeneralTypeData> bg = GeneralDataMgr::getInstance()->GetBaseGeneral(cg.gid);
                if (!bg.get())
                {
                    continue;
                }
                cg.cid = m_cid;
                cg.spic = bg->m_spic;
                cg.name = bg->m_name;
                cg.color = q.getval();
                cg.b_nickname = q.getval();
                cg.m_str = q.getval();
                cg.m_int = q.getval();
                cg.m_tongyu = q.getval();
                cg.m_org_attack = q.getval();
                cg.m_org_wu_fang = q.getval();
                cg.m_org_ce_fang = q.getval();

                cg.m_attack = cg.m_org_attack;
                cg.m_ce_fang = cg.m_org_ce_fang;
                cg.m_wu_fang = cg.m_org_wu_fang;
                
                cg.m_org_hp_max = q.getval();
                cg.m_org_hp_hurt = q.getval();
                std::string combatAttr = q.getstr();
                //解析combatAttr
                cg.m_combat_attribute.load(combatAttr);

                cg.m_hp_org = q.getval();
                cg.m_hp_hurt = q.getval();
                cg.m_inspired = q.getval();

                m_generals.push_back(cg);
            }
            q.free_result();
        }
        //地图数据
        q.get_result("select mapid,stageid from char_zst_maps where cid=" + LEX_CAST_STR(m_cid) + " order by mapid asc, stageid asc");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int mapid = q.getval();
            int stageid = q.getval();

            CharZSTMapData* mp = CharZSTMapsData[mapid].get();
            if (mp == NULL)
            {
                mp = new CharZSTMapData;
                CharZSTMapsData[mapid].reset(mp);
            }
            CharZSTStageData* st = new CharZSTStageData;
            (*mp)[stageid].reset(st);
            st->m_cid = m_cid;
            st->m_mapid = mapid;
            st->m_stageid = stageid;
            st->load();

            if (st->m_stageid == 1)
            {
                if (st->m_mapid == 1 && st->m_state == 0)
                {
                    st->m_state = 1;
                }
                else if (st->m_mapid > 1 && st->m_state == 0)
                {
                    if (checkFinish(st->m_mapid-1))
                    {
                        st->m_state = 1;
                    }
                }
            }
        }
        
        q.free_result();
    }
    else
    {
        q.free_result();
        //cout << "new char_zst cid=" << m_cid << endl;
        q.execute("insert into char_zst (cid) values (" + LEX_CAST_STR(m_cid) + ")");
        CHECK_DB_ERR(q);
        //地图数据初始化
        for(int mapid = 1; mapid <= 3; ++mapid)
        {
            //cout << "new char_zst mapid=" << mapid << endl;
            boost::shared_ptr<CharZSTMapData> tmp;
            tmp.reset(new (CharZSTMapData));
            CharZSTMapsData[mapid] = tmp;
            for(int stageid = 1; stageid <= 10; ++stageid)
            {
                //cout << "new char_zst stageid=" << stageid << endl;
                boost::shared_ptr<CharZSTStageData> tmp2;
                tmp2.reset(new (CharZSTStageData));
                (*tmp.get())[stageid] = tmp2;
                tmp2->m_cid = m_cid;
                tmp2->m_mapid = mapid;
                tmp2->m_stageid = stageid;
                tmp2->m_state = 0;
                if (mapid == 1 && stageid == 1)
                    tmp2->m_state = 1;
                tmp2->m_result_star = 0;
                tmp2->m_stronghold_state.clear();
                tmp2->m_stronghold_state.insert(tmp2->m_stronghold_state.begin(), 5, 0);
                tmp2->m_stronghold_star.clear();
                tmp2->m_stronghold_star.insert(tmp2->m_stronghold_star.begin(), 5, 1);
                tmp2->m_baseStage = Singleton<zstMgr>::Instance().getBaseZSTStage(mapid,stageid);
                tmp2->Save();
            }
        }
    }
}

void char_zst::reset_generals(int cid)
{
    //获得角色信息
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        ERR();
        return;
    }
    cdata->updateAttackDefense();
    //获得角色武将数据
    CharTotalGenerals& char_generals = cdata->GetGenerals();
    //角色阵型信息
    CharZhens& zhens = cdata->GetZhens();
    boost::shared_ptr<ZhenData> zdata = zhens.GetZhen(zhens.GetDefault());
    if (!zdata.get())
    {
        ERR();
        cout<<"default zhen "<<zhens.GetDefault()<<endl;
        return;
    }
    m_generals.clear();

    saveDbJob job;
    job.sqls.push_back("delete from char_zst_generals where cid=" + LEX_CAST_STR(m_cid));
    for (size_t i = 0; i < 9; ++i)
    {
        if (zdata->m_generals[i] > 0)
        {
            boost::shared_ptr<CharGeneralData> sp = char_generals.GetGenral(zdata->m_generals[i]);
            if (sp.get())
            {
                const CharGeneralData& gdata = *sp.get();
                if (gdata.m_baseGeneral.get() && gdata.m_baseSoldier.get())
                {
                    char_zst_general cmg;
                    cmg.pos = i + 1;
                    cmg.id = gdata.m_id;
                    cmg.gid = gdata.m_gid;
                    cmg.cid = gdata.m_cid;
                    cmg.spic = gdata.m_spic;
                    cmg.level = gdata.m_level;
                    cmg.color = gdata.m_color;
                    cmg.b_nickname = gdata.b_nickname;
                    cmg.name = gdata.m_baseGeneral->m_name;
                    //三围
                    cmg.m_str = gdata.m_str;
                    cmg.m_int = gdata.m_int;
                    cmg.m_tongyu = gdata.m_tongyu;
                    if (gdata.m_baowu_type != 0)
                    {
                        switch(gdata.m_baowu_type)
                        {
                            case 1:
                                cmg.m_str += gdata.m_baowu_add;
                                break;
                            case 2:
                                cmg.m_int += gdata.m_baowu_add;
                                break;
                            case 3:
                                cmg.m_tongyu += gdata.m_baowu_add;
                                break;
                            default:
                                break;
                        }
                    }
                    //战斗属性继承自主将
                    cmg.m_combat_attribute = cdata->m_combat_attribute;
                    //cout<<"1 print"<<endl;
                    //cmg.m_combat_attribute.print(true);
                    
                    //战斗属性加上兵种自带的属性
                    cmg.m_combat_attribute += gdata.m_baseSoldier->m_combat_attribute;

                    //cout<<"2 print"<<endl;
                    //cmg.m_combat_attribute.print(true);
                    
                    //战斗属性
                    cmg.m_combat_attribute += gdata.m_combat_attr;
                    //cout<<"3 print"<<endl;
                    //cmg.m_combat_attribute.print(true);
                    
                    //获得练兵数据
                    boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*(cdata.get()));
                    if (ct.get())
                    {
                        //战斗属性加上兵魂属性
                        cmg.m_combat_attribute += ct->_combatAttr;
                        //cout<<"4 print"<<endl;
                        //cmg.m_combat_attribute.print(true);
                    }
                    cmg.m_combat_attribute.enable();

                    if (act_wuli_attack == gdata.m_baseSoldier->m_damage_type)
                    {
                        cmg.m_attack = 2 * cmg.m_str + cdata->getPugong(true) + cmg.m_combat_attribute.skill_add(1) + gdata.m_attack + cmg.m_combat_attribute.soul_add_attack(gdata.m_baseSoldier->m_base_type);
                    }
                    else
                    {
                        cmg.m_attack = 2 * cmg.m_int + cdata->getCegong(true) + cmg.m_combat_attribute.skill_add(3) + gdata.m_attack + cmg.m_combat_attribute.soul_add_attack(gdata.m_baseSoldier->m_base_type);
                    }
                    cmg.m_wu_fang = 7 * cmg.m_str / 5 + cdata->getPufang(true) + cmg.m_combat_attribute.skill_add(2) + gdata.m_pufang + cmg.m_combat_attribute.soul_add_wufang(gdata.m_baseSoldier->m_base_type);
                    cmg.m_ce_fang = 7 * cmg.m_int / 5 + cdata->getCefang(true) + cmg.m_combat_attribute.skill_add(4) + gdata.m_cefang + cmg.m_combat_attribute.soul_add_cefang(gdata.m_baseSoldier->m_base_type);
    
                    /****血量 *******************************************/
                    cmg.m_hp_org = 3*cmg.m_tongyu;   //原始血量
                    if (cmg.m_hp_org <= 0)
                    {
                        cmg.m_hp_org = 1;
                    }
                    //兵器装备加成血量
                    cmg.m_hp_org += (cdata->getBingli(true));
                    //技能加血
                    cmg.m_hp_org += cmg.m_combat_attribute.skill_add(0);
                    cmg.m_hp_org += gdata.m_hp;
                    cmg.m_hp_org += cmg.m_combat_attribute.soul_add_hp(gdata.m_baseSoldier->m_base_type);
                    //成长星级加成
                    if (gdata.m_chengzhang_star.get())
                    {
                        cmg.m_attack += gdata.m_chengzhang_star->gongji;
                        cmg.m_wu_fang += gdata.m_chengzhang_star->fangyu;
                        cmg.m_ce_fang += gdata.m_chengzhang_star->fangyu;
                        cmg.m_hp_org += gdata.m_chengzhang_star->bingli;
                    }
                    //将魂加成
                    if (gdata.m_general_soul)
                    {
                        cmg.m_attack += gdata.m_general_soul->getAttack(gdata.m_color);
                        cmg.m_wu_fang += gdata.m_general_soul->getWufang(gdata.m_color);
                        cmg.m_ce_fang += gdata.m_general_soul->getCefang(gdata.m_color);
                        cmg.m_hp_org += gdata.m_general_soul->getBingli(gdata.m_color);
                    }
                    
                    //武将天赋
                    if (gdata.m_baseGeneral.get())
                    {
                        cmg.m_combat_attribute += gdata.m_baseGeneral->m_new_tianfu.m_combatAttr;
                        //cout<<"5 print"<<endl;
                        //cmg.m_combat_attribute.print(true);
                        if (gdata.m_baseGeneral->m_new_tianfu.m_more_hp)
                        {
                            cmg.m_hp_org = (100 + gdata.m_baseGeneral->m_new_tianfu.m_more_hp) * cmg.m_hp_org / 100;
                        }
                    }
                    
                    //限时增益效果加成
                    int hp_buff = 0, attack_buff = 0, wu_fang_buff = 0, ce_fang_buff = 0;
                    //兵力物攻物防策攻策防
                    for (int i = 0; i < 5; ++i)
                    {
                        switch(i+1)
                        {
                            case 1:
                                hp_buff = cmg.m_hp_org * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                break;
                            case 2:
                                if (act_wuli_attack == gdata.m_baseSoldier->m_damage_type)
                                {
                                    attack_buff = cmg.m_attack * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                }
                                break;
                            case 3:
                                wu_fang_buff = cmg.m_wu_fang * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                break;
                            case 4:
                                if (act_wuli_attack != gdata.m_baseSoldier->m_damage_type)
                                {
                                    attack_buff = cmg.m_attack * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                }
                                break;
                            case 5:
                                ce_fang_buff = cmg.m_ce_fang * (cdata->m_Buffs.buffs[i].m_value) / 100;
                                break;
                            default:
                                break;
                        }
                    }

                    //将星录加成
                    int hp_jxl = 0, cefang_jxl = 0, wufang_jxl = 0, attack_jxl = 0;
                    cdata->m_jxl_buff.total_buff_attr.get_add(cmg.m_attack, cmg.m_hp_org, cmg.m_wu_fang, cmg.m_ce_fang, attack_jxl, hp_jxl, wufang_jxl, cefang_jxl);

                    //皇座称号加成
                    int hp_throne = 0, attack_throne = 0, wu_fang_throne = 0, ce_fang_throne = 0;
                    int throne_per = 0;
                    if (cdata->m_nick.check_nick(nick_throne_start))
                    {
                        throne_per = 8;
                    }
                    else if(cdata->m_nick.check_nick(nick_throne_start + 1))
                    {
                        throne_per = 5;
                    }
                    else if(cdata->m_nick.check_nick(nick_throne_start + 2))
                    {
                        throne_per = 3;
                    }
                    hp_throne = cmg.m_hp_org * throne_per / 100;
                    attack_throne = cmg.m_attack * throne_per / 100;
                    wu_fang_throne = cmg.m_wu_fang * throne_per / 100;
                    ce_fang_throne = cmg.m_ce_fang * throne_per / 100;

                    cmg.m_hp_org += (hp_buff + hp_jxl + hp_throne);
                    cmg.m_attack += (attack_buff + attack_jxl + attack_throne);
                    cmg.m_wu_fang += (wu_fang_buff + wufang_jxl + wu_fang_throne);
                    cmg.m_ce_fang += (ce_fang_buff + cefang_jxl + ce_fang_throne);

                    cdata->m_jxl_buff.total_buff_attr.add_special(cmg.m_combat_attribute);
                    
                    cmg.m_hp_hurt = 0;
                    cmg.m_org_hp_hurt = 0;
                    cmg.m_org_hp_max = cmg.m_hp_org;
                    cmg.m_org_attack = cmg.m_attack;
                    cmg.m_org_wu_fang = cmg.m_wu_fang;
                    cmg.m_org_ce_fang = cmg.m_ce_fang;
                    cmg.m_inspired = 0;
                    m_generals.push_back(cmg);

                    std::string combat_string = "";
                    cmg.m_combat_attribute.save(combat_string);
                    job.sqls.push_back("replace into char_zst_generals (cid,pos,guid,gid,level,color,nickname,str,wisdom,tong,org_attack,org_wufang,org_cefang,org_max_hp,org_hurt_hp,combatAttr,max_hp,hurt_hp,inspired) values ("
                        + LEX_CAST_STR(m_cid) + ","
                        + LEX_CAST_STR(cmg.pos) + ","
                        + LEX_CAST_STR(cmg.id) + ","
                        + LEX_CAST_STR(cmg.gid) + ","
                        + LEX_CAST_STR(cmg.level) + ","
                        //+ LEX_CAST_STR(cmg.spic) + ","
                        + LEX_CAST_STR(cmg.color) + ","
                        + LEX_CAST_STR(cmg.b_nickname) + ","
                        + LEX_CAST_STR(cmg.m_str) + ","
                        + LEX_CAST_STR(cmg.m_int) + ","
                        + LEX_CAST_STR(cmg.m_tongyu) + ","
                        + LEX_CAST_STR(cmg.m_org_attack) + ","
                        + LEX_CAST_STR(cmg.m_org_wu_fang) + ","
                        + LEX_CAST_STR(cmg.m_org_ce_fang) + ","
                        + LEX_CAST_STR(cmg.m_org_hp_max) + ","
                        + LEX_CAST_STR(cmg.m_org_hp_hurt) + ",'"
                        + LEX_CAST_STR(combat_string) + "',"
                        + LEX_CAST_STR(cmg.m_hp_org) + ","
                        + LEX_CAST_STR(cmg.m_hp_hurt) + ","
                        + LEX_CAST_STR(cmg.m_inspired) + ")");
                }
            }
        }
    }
    InsertSaveDb(job);
}

int char_zst::reset_stage(int mapid, int stageid)
{
    if (CharZSTMapsData.find(mapid) != CharZSTMapsData.end() && CharZSTMapsData[mapid].get())
    {
        boost::shared_ptr<CharZSTMapData> tmp = CharZSTMapsData[mapid];
        if ((*tmp.get()).find(stageid) != (*tmp.get()).end() && (*tmp.get())[stageid].get())
        {
            boost::shared_ptr<CharZSTStageData> tmp2 = (*tmp.get())[stageid];
            if (tmp2->m_state != 0)
            {
                tmp2->m_stronghold_state.clear();
                tmp2->m_stronghold_state.insert(tmp2->m_stronghold_state.begin(), 5, 0);
                tmp2->m_stronghold_star.clear();
                tmp2->m_stronghold_star.insert(tmp2->m_stronghold_star.begin(), 5, my_random(1,5));
                tmp2->Save();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

//刷新星级
int char_zst::refreshStar(CharData& cdata, int mapid, int stageid, int type, json_spirit::Object& robj)
{
    //当前在挑战场景才能刷新
    if (mapid != m_cur_map || stageid != m_cur_stage)
        return HC_ERROR;
    //刷新次数判断
    bool need_gold = true;
    int refresh_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_refresh);
    if (refresh_times < iZSTFreeRefreshTimes)
        need_gold = false;
    if (CharZSTMapsData.find(mapid) != CharZSTMapsData.end() && CharZSTMapsData[mapid].get())
    {
        boost::shared_ptr<CharZSTMapData> tmp = CharZSTMapsData[mapid];
        if ((*tmp.get()).find(stageid) != (*tmp.get()).end() && (*tmp.get())[stageid].get())
        {
            boost::shared_ptr<CharZSTStageData> tmp2 = (*tmp.get())[stageid];
            //if (tmp2->m_state != 1)
            //    return HC_ERROR;

            //根据刷新类型确定消费
            int cost_gold = iZSTRefreshCost, refresh_min = 1;
            if (type == 1)
            {
                if (refresh_times < iZSTFreeRefreshTimes)
                {
                    cost_gold = 0;
                    cdata.setExtraData(char_data_type_daily, char_data_daily_zst_refresh, refresh_times+1);
                }
                else
                {
                    cost_gold = iZSTRefreshCost;
                }
                refresh_min = 1;
            }
            else// if (type == 2)
            {
                cost_gold = iZSTRefresh5Cost;
                refresh_min = 5;
            }

            if (cost_gold)
            {
                //扣除金币
                if (cdata.addGold(-cost_gold) < 0)
                {
                    return HC_ERROR_NOT_ENOUGH_GOLD;
                }
                add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, cost_gold, gold_cost_for_zst_refresh, cdata.m_union_id, cdata.m_server_id);
                #ifdef QQ_PLAT
                gold_cost_tencent(&cdata,cost_gold,gold_cost_for_zst_refresh);
                #endif
                cdata.NotifyCharData();
            }
            //刷新星级
            for (int i = 0; i < 5; ++i)
            {
                if (tmp2->m_stronghold_state[i] == 0)
                {
                    tmp2->m_stronghold_star[i] = my_random(refresh_min, 5);
                }
            }
               
            //保存关卡数据
            tmp2->Save();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int char_zst::updateTotalStar()
{
    m_total_star = 0;
    std::map<int, boost::shared_ptr<CharZSTMapData> >::iterator it = CharZSTMapsData.begin();
    while (it != CharZSTMapsData.end() && it->second.get())
    {
        boost::shared_ptr<CharZSTMapData> tmp = it->second;
        std::map<int, boost::shared_ptr<CharZSTStageData> >::iterator it_i = (*tmp.get()).begin();
        while (it_i != (*tmp.get()).end() && it_i->second.get())
        {
            boost::shared_ptr<CharZSTStageData> tmp2 = it_i->second;
            if (tmp2->m_state == 2)
            {
                m_total_star += tmp2->m_result_star;
            }
            ++it_i;
        }
        ++it;
    }
}

bool char_zst::checkFinish(int mapid)
{
    std::map<int, boost::shared_ptr<CharZSTMapData> >::iterator it = CharZSTMapsData.find(mapid);
    if (it != CharZSTMapsData.end() && it->second.get())
    {
        boost::shared_ptr<CharZSTMapData> md = it->second;
        CharZSTMapData::iterator itm = (*md).begin();
        while (itm != (*md).end())
        {
            if (itm->second.get())
            {
                if (itm->second->m_state != 2)
                {
                    //ERR();
                    //cout<<"char_zst::checkFinish,cid:"<<m_cid<<",mapid:"<<mapid<<",stage:"<<itm->second->m_stageid<<",state:"<<itm->second->m_state<<endl;
                    return false;
                }
            }
            else
            {
                //ERR();
                //cout<<"char_zst::checkFinish,cid:"<<m_cid<<",mapid:"<<mapid<<",NULL"<<endl;
                return false;
            }
            ++itm;
        }
        return true;
    }
    //cout<<"char_zst::checkFinish 3,cid:"<<m_cid<<",mapid:"<<mapid<<endl;
    return false;
}

//挑战
int char_zst::challenge(CharData& cdata, int mapid, int stageid, int pos, json_spirit::Object& robj)
{
    if (mapid != m_cur_map || stageid != m_cur_stage)
        return HC_ERROR;
    if (CharZSTMapsData.find(mapid) != CharZSTMapsData.end() && CharZSTMapsData[mapid].get())
    {
        boost::shared_ptr<CharZSTMapData> tmp = CharZSTMapsData[mapid];
        if ((*tmp.get()).find(stageid) != (*tmp.get()).end() && (*tmp.get())[stageid].get() && pos >= 1 && pos <= 5)
        {
            boost::shared_ptr<CharZSTStageData> tmp2 = (*tmp.get())[stageid];
            int star = tmp2->m_stronghold_star[pos-1];
            if (tmp2->m_baseStage.get() && tmp2->m_baseStage->_baseStrongholds[(star-1)*5+pos-1].get())
            {
                if (tmp2->m_stronghold_state[pos-1] == 0)
                {
                    int ret = HC_SUCCESS;
                    Combat* pCombat = createZSTCombat(tmp2->m_cid, star, tmp2->m_baseStage->_baseStrongholds[(star-1)*5+pos-1]->_id, ret, false);
                    if (HC_SUCCESS == ret && pCombat)
                    {
                        //立即返回战斗双方的信息
                        pCombat->setCombatInfo();
                        InsertCombat(pCombat);
                        std::string sendMsg = "{\"cmd\":\"attack\",\"s\":200,\"getBattleList\":" + pCombat->getCombatInfoText() + "}";
                        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata.m_name);
                        if (account.get())
                        {
                            account->Send(sendMsg);
                        }
                    }
                    return HC_SUCCESS;
                }
            }
        }
    }
    return HC_ERROR;
}

void char_zst::combatEnd(Combat* pCombat)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        return;
    }
    if (attacker_win == pCombat->m_state)
    {
        boost::shared_ptr<base_ZST_Stronghold> stronghold_data = m_handle.getBaseZSTStronghold(pCombat->m_type_id);
        base_ZST_Stronghold *bShold = stronghold_data.get();
        if (NULL == bShold)
        {
            ERR();
        }
        //战胜更新相关数据
        Army* winner_army = pCombat->m_attacker;
        //更新武将数据
        for (std::list<char_zst_general>::iterator it = m_generals.begin(); it != m_generals.end(); ++it)    //队伍里面的武将
        {
            char_zst_general& g = *it;
            //g.m_hp_now = g.m_hp_org;
            iGeneral* combat_g = winner_army->GetGeneral(g.pos);
            if (combat_g)
            {
                int cur_hp = combat_g->Hp();
                if (cur_hp > 0)
                {
                    int inspired = combat_g->total_inspired();
                    if (inspired != 0 && inspired != -100)
                    {
                        cur_hp = cur_hp * 100 / (100 + inspired);
                        if (cur_hp <= 0)
                        {
                            cur_hp = 1;
                        }
                    }
                }
                if (cur_hp > 0)
                {
                    int hurt = combat_g->MaxHp() - cur_hp;
                    if (combat_g->MaxHp() == g.m_hp_org)
                    {
                        g.m_hp_hurt = hurt;
                    }
                    else
                    {
                        g.m_hp_hurt = hurt * g.m_hp_org / combat_g->MaxHp();                            
                    }
                    
                    if (combat_g->MaxHp() == g.m_org_hp_max)
                    {
                        g.m_org_hp_hurt = hurt;
                    }
                    else
                    {
                        g.m_org_hp_hurt = hurt * g.m_org_hp_max / combat_g->MaxHp();                            
                    }
                }
                else
                {
                    g.m_hp_hurt = g.m_hp_org;
                    g.m_org_hp_hurt = g.m_org_hp_max;
                }
            }
            else
            {
                g.m_hp_hurt = g.m_hp_org;
                g.m_org_hp_hurt = g.m_org_hp_max;
            }
        }
        SaveGenerals();

        //更新地图数据
        bool end = true;
        int star = 0;
        if (CharZSTMapsData.find(bShold->_mapid) != CharZSTMapsData.end() && CharZSTMapsData[bShold->_mapid].get())
        {
            boost::shared_ptr<CharZSTMapData> tmp = CharZSTMapsData[bShold->_mapid];
            if ((*tmp.get()).find(bShold->_stageid) != (*tmp.get()).end() && (*tmp.get())[bShold->_stageid].get())
            {
                boost::shared_ptr<CharZSTStageData> tmp2 = (*tmp.get())[bShold->_stageid];
                if (tmp2->m_baseStage.get() && bShold->_pos >= 1 && bShold->_pos <= 5)
                {
                    //设置为击败
                    if (tmp2->m_stronghold_state[bShold->_pos-1] == 0)
                    {
                        tmp2->m_stronghold_state[bShold->_pos-1] = 1;
                    }
                    star = tmp2->m_stronghold_star[bShold->_pos-1];
                }
                tmp2->Save();
                for (int i = 0; i < tmp2->m_stronghold_state.size(); ++i)
                {
                    if (tmp2->m_stronghold_state[i] != 1)
                        end = false;
                }
            }
        }
        if (end)//通关
        {
            //通知次数变化
            int challenge_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_zst_challenge);
            int buy_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge);
            int left = iZSTChallengeTimes + buy_times - challenge_times;
            cdata->notifyEventState(top_level_event_zst, 0, left);
            
            //清除数据
            Clear();
            //更新地图数据
            if (CharZSTMapsData.find(bShold->_mapid) != CharZSTMapsData.end() && CharZSTMapsData[bShold->_mapid].get())
            {
                boost::shared_ptr<CharZSTMapData> tmp = CharZSTMapsData[bShold->_mapid];
                if ((*tmp.get()).find(bShold->_stageid) != (*tmp.get()).end() && (*tmp.get())[bShold->_stageid].get())
                {
                    boost::shared_ptr<CharZSTStageData> tmp2 = (*tmp.get())[bShold->_stageid];
                    //结算通关星级
                    if (tmp2->m_state == 1)
                    {
                        tmp2->m_state = 2;
                    }
                    //开启下一个
                    if ((*tmp.get()).find(bShold->_stageid+1) != (*tmp.get()).end() && (*tmp.get())[bShold->_stageid+1].get() && (*tmp.get())[bShold->_stageid+1]->m_state == 0)
                    {
                        (*tmp.get())[bShold->_stageid+1]->m_state = 1;
                        (*tmp.get())[bShold->_stageid+1]->Save();
                    }
                    //开启下一张图
                    else if ((*tmp.get()).find(bShold->_stageid+1) == (*tmp.get()).end())
                    {
                        if (CharZSTMapsData.find(bShold->_mapid+1) != CharZSTMapsData.end() && CharZSTMapsData[bShold->_mapid+1].get())
                        {
                            CharZSTMapData& zstMap = *(CharZSTMapsData[bShold->_mapid+1].get());
                            if (zstMap.find(1) != zstMap.end() && zstMap[1].get())
                            {
                                CharZSTStageData* zstStage = zstMap[1].get();
                                if (zstStage->m_state == 0)
                                {
                                    zstStage->m_state = 1;
                                    zstStage->Save();
                                }
                            }
                        }
                    }
                    if (tmp2->getStar() > tmp2->m_result_star)
                    {
                        tmp2->m_result_star = tmp2->getStar();
                        m_star_update_time = time(NULL);
                    }
                    tmp2->Save();
                    //根据星级给场景通关奖励
                    std::list<Item> getItems;                    
                    lootMgr::getInstance()->getZSTLoots(bShold->_mapid,bShold->_stageid,tmp2->m_result_star, getItems,0);

                    if (star)
                    {
                        int c = star / 2;
                        if (c == 0)
                        {
                            c = 1;
                        }
                        bool bGived = false;
                        for (std::list<Item>::iterator it = getItems.begin(); it != getItems.end(); ++it)
                        {
                            if (it->type == item_type_treasure && it->id == treasure_type_general_soul)
                            {
                                bGived = true;
                                it->nums += c;
                                break;
                            }
                        }
                        if (!bGived)
                        {
                            Item itm;
                            itm.type = item_type_treasure;
                            itm.id = treasure_type_general_soul;
                            itm.nums = c;
                            getItems.push_back(itm);
                        }
                    }
                    giveLoots(cdata.get(), getItems, 0, cdata->m_level, 0, pCombat, NULL, true, give_zst);
                    updateTotalStar();
                    Save();
                }
            }
        }
        else
        {
            if (star)
            {
                int c = star / 2;
                if (c == 0)
                {
                    c = 1;
                }
                //根据星级给单关奖励
                std::list<Item> getItems;
                Item itm;
                itm.type = item_type_treasure;
                itm.id = treasure_type_general_soul;
                itm.nums = c;
                getItems.push_back(itm);
                giveLoots(cdata.get(), getItems, 0, cdata->m_level, 0, pCombat, NULL, true, give_zst);
            }
        }
    }
    else
    {
        //战斗失败结束
        //notifyFail();
        Clear();
        //通知次数变化
        int challenge_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_zst_challenge);
        int buy_times = cdata->queryExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge);
        int left = iZSTChallengeTimes + buy_times - challenge_times;
        cdata->notifyEventState(top_level_event_zst, 0, left);
    }
}

void char_zst::Clear()
{
    m_cur_map = 0;
    m_cur_stage = 0;
    m_generals.clear();
    saveDbJob job;
    job.sqls.push_back("update char_zst set mapid=0,stageid=0 where cid=" + LEX_CAST_STR(m_cid));
    job.sqls.push_back("delete from char_zst_generals where cid=" + LEX_CAST_STR(m_cid));
    InsertSaveDb(job);
}

void char_zst::Save()
{
    InsertSaveDb("update char_zst set mapid=" + LEX_CAST_STR(m_cur_map)
        + ",stageid=" + LEX_CAST_STR(m_cur_stage)
        + ",star_reward=" + LEX_CAST_STR(m_cur_star_reward)
        + ",star=" + LEX_CAST_STR(m_total_star)
        + ",star_update_time=" + LEX_CAST_STR(m_star_update_time)
        + " where cid=" + LEX_CAST_STR(m_cid));
}

void char_zst::SaveGenerals()
{
    for (std::list<char_zst_general>::iterator it = m_generals.begin(); it != m_generals.end(); ++it)    //队伍里面的武将
    {
        char_zst_general& g = *it;
        g.Save();
    }
}

int base_ZST_Stronghold::load()
{
    Query q(GetDb());
    q.get_result("select pos,name,spic,stype,hp,attack,pufang,cefang,str,wisdom,skill from base_zst_armys_generals where id=" + LEX_CAST_STR(_id)+ " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        if (pos >= 9)
        {
            pos = 9;
        }
        else if (pos < 1)
        {
            pos = 1;
        }
        boost::shared_ptr<StrongholdGeneralData> sg;
        if (!(m_generals[pos-1].get()))
        {
            sg.reset(new (StrongholdGeneralData));
            m_generals[pos-1] = sg;
        }
        else
        {
            sg = m_generals[pos-1];
        }
        sg->m_pos = pos;
        sg->m_name = q.getstr();
        sg->m_spic = q.getval();
        sg->m_stype = q.getval();
        sg->m_hp = q.getval();
        sg->m_attack = q.getval();
        sg->m_pufang = q.getval();
        sg->m_cefang = q.getval();
        sg->m_str = q.getval();
        sg->m_int = q.getval();
        sg->m_speSkill = GeneralDataMgr::getInstance()->getSpeSkill(q.getval());
        sg->m_level = _level;

        sg->m_baseSoldier = GeneralDataMgr::getInstance()->GetBaseSoldier(sg->m_stype);
    }
    q.free_result();
    return 0;
}

std::string total_star_rewards::toString(int level)
{
    std::string str_rewards = "";
    for (std::list<Item>::iterator it = _rewards.begin(); it != _rewards.end(); ++it)
    {
        if (str_rewards != "")
        {
            str_rewards += ",";
        }
        str_rewards += it->toString(false, level);
    }
    return str_rewards;
}

zstMgr::zstMgr()
{
    lootMgr::getInstance();
    Query q(GetDb());
    max_map = 0;
    q.get_result("select id,name from base_zst_maps where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        if (mapid < 1 || mapid > 10)
        {
            ERR();
            continue;
        }
        if (!m_base_maps[mapid-1].get())
        {
            m_base_maps[mapid-1].reset(new base_ZST_Map);
        }
        m_base_maps[mapid-1]->id = mapid;
        m_base_maps[mapid-1]->name = q.getstr();
        max_map = mapid;
    }
    q.free_result();
    q.get_result("select mapid,id,name,spic,needAttack from base_zst_stages where 1 order by mapid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int mapid = q.getval();
        if (!m_base_maps[mapid-1].get())
        {
            ERR();
            continue;
        }
        int stage = q.getval();
        if (stage < 1 || stage > 10)
        {
            ERR();
            continue;
        }
        if (!m_base_maps[mapid-1]->stages[stage-1].get())
        {
            m_base_maps[mapid-1]->stages[stage-1].reset(new base_ZST_Stage);
        }
        m_base_maps[mapid-1]->stages[stage-1]->mapid = mapid;
        m_base_maps[mapid-1]->stages[stage-1]->id = stage;
        m_base_maps[mapid-1]->stages[stage-1]->name = q.getstr();
        m_base_maps[mapid-1]->stages[stage-1]->spic = q.getval();
        m_base_maps[mapid-1]->stages[stage-1]->needAttack = q.getval();
        m_base_maps[mapid-1]->stages[stage-1]->_baseMap.reset();
        m_base_maps[mapid-1]->stages[stage-1]->_baseMap = m_base_maps[mapid-1];
    }
    q.free_result();
    q.get_result("select id,star,mapid,stageid,pos,level,spic,name,needAttack,hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry from base_zst_armys where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        int star = q.getval();
        int mapid = q.getval();
        int stageid = q.getval();
        int pos = q.getval();
        boost::shared_ptr<base_ZST_Stronghold> m_data;
        m_data.reset(new base_ZST_Stronghold);
        m_data->_id = id;
        m_data->_star = star;
        m_data->_mapid = mapid;
        m_data->_stageid = stageid;
        m_data->_pos = pos;
        m_data->_level = q.getval();
        m_data->_spic = q.getval();
        m_data->_name = q.getstr();
        m_data->needAttack = q.getval();
        //特性hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry
        m_data->m_combat_attribute.special_resist(special_attack_dodge, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_baoji, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_shipo, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_parry, 10 * q.getval());
        m_data->m_combat_attribute.special_attack(special_attack_dodge, 10 * q.getval());
        m_data->m_combat_attribute.special_resist(special_attack_baoji, 10 * q.getval());
        m_data->m_combat_attribute.special_resist(special_attack_shipo, 10 * q.getval());
        m_data->m_combat_attribute.special_resist(special_attack_parry, 10 * q.getval());
        m_data->m_combat_attribute.enable();
        //加载武将
        m_data->load();
        m_stronghold_data_map[id] = m_data;

        boost::shared_ptr<base_ZST_Map> bm = m_base_maps[mapid-1];
        if (bm.get() && (m_data->_stageid >=1 && m_data->_stageid <=10) && bm->stages[m_data->_stageid-1].get())
        {
            if (m_data->_pos>=1 && m_data->_pos<=5 && m_data->_star >= 1 && m_data->_star <= 5)
            {
                bm->stages[m_data->_stageid -1]->_baseStrongholds[(m_data->_star-1)*5+m_data->_pos-1] = m_data;
            }
        }
        else
        {
            ERR();
        }
    }
    q.free_result();
    //加载星级奖励
    q.get_result("select id,need_star,reward_type,reward_id,reward_nums from base_zst_star_reward where 1 order by id");
    while (q.fetch_row())
    {
        int id = q.getval();
        int need_star = q.getval();
        boost::shared_ptr<total_star_rewards> p_award = m_total_star_rewards[id];
        if (!p_award.get())
        {
            p_award.reset(new total_star_rewards);
            p_award->_needstar = need_star;
            m_total_star_rewards[id] = p_award;
        }
        Item i;
        i.type = q.getval();
        i.id = q.getval();
        i.nums = q.getval();
        p_award->_rewards.push_back(i);
    }
    q.free_result();
}

boost::shared_ptr<char_zst> zstMgr::getChar(int cid)
{
    std::map<int, boost::shared_ptr<char_zst> >::iterator it = m_char_datas.find(cid);
    if (it != m_char_datas.end())
    {
        return it->second;
    }
    boost::shared_ptr<char_zst> r(new char_zst(cid, *this));
    m_char_datas[cid] = r;
    r->load();
    return r;
}

//基础关卡信息
boost::shared_ptr<base_ZST_Stronghold> zstMgr::getBaseZSTStronghold(int strongholdid)
{
    std::map<int, boost::shared_ptr<base_ZST_Stronghold> >::iterator it = m_stronghold_data_map.find(strongholdid);
    if (it != m_stronghold_data_map.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<base_ZST_Stronghold> gd;
        gd.reset();
        return gd;
    }
}

boost::shared_ptr<base_ZST_Stage> zstMgr::getBaseZSTStage(int mapid, int stageid)
{
    if (mapid >= 1 && mapid <= 10 && m_base_maps[mapid-1].get() && stageid >= 1 && stageid <=10
        && m_base_maps[mapid-1]->stages[stageid-1].get())
    {
        return m_base_maps[mapid-1]->stages[stageid-1];
    }
    else
    {
        boost::shared_ptr<base_ZST_Stage> gd;
        gd.reset();
        return gd;
    }
}

//获得地图描述信息
int zstMgr::GetMapMemo(int mapid, std::string& name)
{
    boost::shared_ptr<base_ZST_Map> bm = m_base_maps[mapid-1];
    if (bm.get())
    {
        name = bm->name;
        return 0;
    }
    else
    {
        ERR();
        return -1;
    }
}

int zstMgr::GetStageMemo(int mapid, int stageid, std::string& name)
{
    boost::shared_ptr<base_ZST_Stage> bs = getBaseZSTStage(mapid, stageid);
    if (bs.get())
    {
        name = bs->name;
        return 0;
    }
    else
    {
        //ERR();
        //cout<<"zstMgr::GetStageMemo("<<mapid<<","<<stageid<<")"<<endl;
        return -1;
    }
}

int zstMgr::queryZstMapInfo(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int mapid = 0;
        READ_INT_FROM_MOBJ(mapid, o, "mapid");
        if (mapid < 0 || mapid > 10)
            return HC_ERROR;
        if (mapid == 0)
        {
            if (cz->m_cur_map > 0)
                mapid = cz->m_cur_map;
            else
            {
                for(int i = 1; i <= 10; ++i)
                {
                    if (!cz->checkFinish(i))
                    {
                        mapid = i;
                        break;
                    }
                }
                if (mapid == 0)
                    mapid = 10;
            }
        }
        json_spirit::Array maptempo;
        std::map<int, boost::shared_ptr<CharZSTMapData> >::iterator it = cz->CharZSTMapsData.find(mapid);
        if (it != cz->CharZSTMapsData.end() && it->second.get())
        {
            boost::shared_ptr<CharZSTMapData> md = it->second;
            CharZSTMapData::iterator itm = (*md).begin();
            while (itm != (*md).end())
            {
                if (itm->second.get() && itm->second->m_baseStage.get())
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("id", itm->first));
                    obj.push_back( Pair("name", itm->second->m_baseStage->name));
                    std::string name = "";
                    if (GetStageMemo(mapid,itm->first - 1,name) == 0)
                        obj.push_back( Pair("pre_name", name));
                    obj.push_back( Pair("spic", itm->second->m_baseStage->spic));
                    obj.push_back( Pair("needAttack", itm->second->m_baseStage->needAttack));
                    obj.push_back( Pair("state", itm->second->m_state));
                    obj.push_back( Pair("result_star", itm->second->m_result_star));
                    if (itm->second->m_mapid == cz->m_cur_map && itm->second->m_stageid == cz->m_cur_stage)
                    {
                        obj.push_back( Pair("challenge", 1));
                    }
                    maptempo.push_back(obj);
                }
                ++itm;
            }
        }
        robj.push_back( Pair("list", maptempo));
        
        //掉落信息
        std::list<Item> m_Item_list;
        lootMgr::getInstance()->getZSTMapLootsInfo(mapid, m_Item_list);
        if (m_Item_list.size() > 0)
        {
            json_spirit::Array list;
            std::list<Item>::iterator it_l = m_Item_list.begin();
            while (it_l != m_Item_list.end())
            {
                json_spirit::Object obj;
                it_l->toObj(obj);
                list.push_back(obj);
                ++it_l;
            }
            robj.push_back( Pair("loot_list", list) );
        }
        //领取信息
        boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
        if (cz.get())
        {
            //是否已经领取过了
            int idx = char_data_zst_map_award_start + mapid;
            int get = cdata.queryExtraData(char_data_type_normal, idx);
            if (get)
            {
                robj.push_back( Pair("loot_state", 2) );
            }
            else if (cz->checkFinish(mapid))
            {
                robj.push_back( Pair("loot_state", 1) );
            }
            else
            {
                robj.push_back( Pair("loot_state", 0) );
            }
        }

        //地图信息
        std::string name = "";
        int ret = GetMapMemo(mapid,name);
        robj.push_back( Pair("max_mapid", max_map));
        robj.push_back( Pair("mapid", mapid));
        if (ret == 0)
            robj.push_back( Pair("mapname", name));
        robj.push_back( Pair("challenge_mapid", cz->m_cur_map));
        int challenge_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_challenge);
        int buy_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge);
        if (cz->m_cur_map > 0 && cz->m_cur_stage > 0)
        {
            --challenge_times;
        }
        robj.push_back( Pair("challengeNums", challenge_times) );
        robj.push_back( Pair("buy_challenge_cost", getZstBuyCost(buy_times)) );
        robj.push_back( Pair("totalNums", iZSTChallengeTimes + buy_times) );
        robj.push_back( Pair("totalStar", cz->m_total_star) );
        int star_ranking = 1;
        Query q(GetDb());
        q.get_result("select count(*) from char_zst where star>" + LEX_CAST_STR(cz->m_total_star));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            star_ranking += q.getval();
        }
        q.free_result();
        q.get_result("select count(*) from char_zst where star=" + LEX_CAST_STR(cz->m_total_star) + " and star_update_time<" + LEX_CAST_STR(cz->m_star_update_time));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            star_ranking += q.getval();
        }
        q.free_result();
        robj.push_back( Pair("star_ranking", star_ranking) );
        int get_reward_id = cz->m_cur_star_reward + 1;
        if (m_total_star_rewards.find(get_reward_id) != m_total_star_rewards.end() && m_total_star_rewards[get_reward_id].get())
        {
            robj.push_back( Pair("nextRewardid", get_reward_id) );
            robj.push_back( Pair("nextRewardStar", m_total_star_rewards[get_reward_id]->_needstar) );
            robj.push_back( Pair("nextRewardCanget", cz->m_total_star >= m_total_star_rewards[get_reward_id]->_needstar) );
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int zstMgr::queryZstStageInfo(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int mapid = 0, stageid = 0;
        READ_INT_FROM_MOBJ(mapid, o, "mapid");
        READ_INT_FROM_MOBJ(stageid, o, "stageid");

        if (cz->m_cur_map == 0 && cz->m_cur_stage == 0)
        {
            int challenge_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_challenge);
            int buy_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge);
            if (challenge_times >= buy_times + iZSTChallengeTimes)
                return HC_ERROR;
            //开始挑战减少次数
            int ret = cz->reset_stage(mapid,stageid);
            if (ret != HC_SUCCESS)
                return HC_ERROR;
            cz->m_cur_map = mapid;
            cz->m_cur_stage = stageid;
            cz->reset_generals(cdata.m_id);
            cdata.setExtraData(char_data_type_daily, char_data_daily_zst_challenge, ++challenge_times);
            cz->Save();
        }
        else if(cz->m_cur_map != mapid || cz->m_cur_stage != stageid)
        {
            return HC_ERROR;
        }

        std::map<int, boost::shared_ptr<CharZSTMapData> >::iterator it = cz->CharZSTMapsData.find(mapid);
        if (it != cz->CharZSTMapsData.end())
        {
            boost::shared_ptr<CharZSTMapData> md = it->second;
            CharZSTMapData::iterator itm = (*md).find(stageid);
            if (itm != (*md).end() && itm->second.get() && itm->second->m_baseStage.get())
            {
                json_spirit::Array stronghold_array;
                for (size_t i = 0; i < 5; ++i)
                {
                    int star = itm->second->m_stronghold_star[i];
                    base_ZST_Stronghold* shold = itm->second->m_baseStage->_baseStrongholds[(star-1)*5+i].get();
                    if (shold)
                    {
                        json_spirit::Object stronghold;
                        stronghold.push_back( Pair("pos", i + 1));
                        stronghold.push_back( Pair("id", shold->_id));
                        stronghold.push_back( Pair("name", shold->_name));
                        stronghold.push_back( Pair("level", shold->_level));
                        stronghold.push_back( Pair("model", shold->_spic));
                        stronghold.push_back( Pair("needAttack", shold->needAttack));
                        stronghold.push_back( Pair("state", itm->second->m_stronghold_state[i]));
                        stronghold.push_back( Pair("star", itm->second->m_stronghold_star[i]));
                        stronghold_array.push_back(stronghold);
                    }
                }
                robj.push_back( Pair("list", stronghold_array));
                
                int result_star = itm->second->getStar();
                robj.push_back( Pair("result_star", result_star));
                //掉落信息
                std::list<Item> m_Item_list;
                lootMgr::getInstance()->getZSTLootsInfo(itm->second->m_mapid,itm->second->m_stageid, result_star, m_Item_list);
                if (m_Item_list.size() > 0)
                {
                    json_spirit::Array list;
                    std::list<Item>::iterator it_l = m_Item_list.begin();
                    while (it_l != m_Item_list.end())
                    {
                        json_spirit::Object obj;
                        it_l->toObj(obj);
                        list.push_back(obj);
                        ++it_l;
                    }
                    robj.push_back( Pair("loot_list", list) );
                }
            }
        }
        
        json_spirit::Array list;
        for (std::list<char_zst_general>::iterator it = cz->m_generals.begin(); it != cz->m_generals.end(); ++it)    //队伍里面的武将
        {
            char_zst_general& g = *it;
            json_spirit::Object g_obj;
            g_obj.push_back( Pair("name", g.name) );
            g_obj.push_back( Pair("level", g.level) );
            g_obj.push_back( Pair("quality", g.color) );
            g_obj.push_back( Pair("spic", g.spic) );
            g_obj.push_back( Pair("org_hp", g.m_hp_org) );
            g_obj.push_back( Pair("now_hp", g.m_hp_org-g.m_hp_hurt) );
            list.push_back(g_obj);
        }
        robj.push_back( Pair("general_list", list) );
        
        std::string name = "";
        int ret = GetMapMemo(mapid,name);
        robj.push_back( Pair("mapid", mapid));
        if (ret == 0)
            robj.push_back( Pair("mapname", name));
        ret = GetStageMemo(mapid,stageid,name);
        robj.push_back( Pair("stageid", stageid));
        if (ret == 0)
            robj.push_back( Pair("stagename", name));
        //刷新信息
        json_spirit::Object refresh_obj;
        int refresh_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_refresh);
        refresh_obj.push_back( Pair("free_times", iZSTFreeRefreshTimes - refresh_times));
        refresh_obj.push_back( Pair("refresh_cost", iZSTRefreshCost));
        refresh_obj.push_back( Pair("refresh_5cost", iZSTRefresh5Cost - refresh_times));
        robj.push_back( Pair("refresh_obj", refresh_obj));
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int zstMgr::refreshZstStar(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int mapid = 0, stageid = 0, type = 0;
        READ_INT_FROM_MOBJ(mapid, o, "mapid");
        READ_INT_FROM_MOBJ(stageid, o, "stageid");
        READ_INT_FROM_MOBJ(type, o, "type");

        return cz->refreshStar(cdata, mapid, stageid, type, robj);
    }
    return HC_ERROR;
}

int zstMgr::buyZstChallenge(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int buy_times = cdata.queryExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge);
        int buy_cost = getZstBuyCost(buy_times);
        //扣除金币
        if (cdata.addGold(-buy_cost) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        add_statistics_of_gold_cost(cdata.m_id, cdata.m_ip_address, buy_cost, gold_cost_for_zst_buy, cdata.m_union_id, cdata.m_server_id);
        #ifdef QQ_PLAT
        gold_cost_tencent(&cdata,buy_cost,gold_cost_for_zst_buy);
        #endif
        cdata.NotifyCharData();
        cdata.setExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge, ++buy_times);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int zstMgr::getZstStarReward(CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int get_reward_id = cz->m_cur_star_reward + 1;
        if (m_total_star_rewards.find(get_reward_id) != m_total_star_rewards.end() && m_total_star_rewards[get_reward_id].get())
        {
            if (cz->m_total_star >= m_total_star_rewards[get_reward_id]->_needstar)
            {
                std::list<Item> getItems;
                getItems = m_total_star_rewards[get_reward_id]->_rewards;
                giveLoots(&cdata, getItems, 0, cdata.m_level, 0, NULL, &robj, true, give_zst);
                cz->m_cur_star_reward = get_reward_id;
                cz->Save();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

int zstMgr::queryZstStarReward(CharData& cdata, json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::map<int, boost::shared_ptr<total_star_rewards> >::iterator it = m_total_star_rewards.begin();
    while (it != m_total_star_rewards.end())
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("need_star", it->second->_needstar) );
            std::string memo = it->second->toString(cdata.m_level);
            obj.push_back( Pair("reward_memo", memo) );
            list.push_back(obj);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int zstMgr::getZstMapReward(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int mapid = 0;
        READ_INT_FROM_MOBJ(mapid, o, "mapid");
        if (cz->checkFinish(mapid))
        {
            //是否已经领取过了
            int idx = char_data_zst_map_award_start + mapid;
            int get = cdata.queryExtraData(char_data_type_normal, idx);
            if (get)
            {
                return HC_ERROR;
            }
            // 通关奖励
            std::list<Item> items;
            lootMgr::getInstance()->getZSTMapLoots(mapid, items, 0);
            //给东西
            giveLoots(&cdata, items, 0, cdata.m_level, 0, NULL, &robj, true, give_zst);
            //设置已经领取
            cdata.setExtraData(char_data_type_normal, idx, 1);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int zstMgr::ZstChallenge(CharData& cdata, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<char_zst> cz = getChar(cdata.m_id);
    if (cz.get())
    {
        int mapid = 0, stageid = 0, pos = 0;
        READ_INT_FROM_MOBJ(mapid, o, "mapid");
        READ_INT_FROM_MOBJ(stageid, o, "stageid");
        READ_INT_FROM_MOBJ(pos, o, "pos");
        return cz->challenge(cdata, mapid, stageid, pos, robj);
    }
    return HC_ERROR;
}

int zstMgr::combatResult(Combat* pCombat)
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (combat_zst != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(pCombat->m_attacker->getCharId());
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<char_zst> cz = getChar(cdata->m_id);
    if (cz.get() && cz->m_cur_map > 0 && cz->m_cur_stage > 0)
    {
        cz->combatEnd(pCombat);
    }
    pCombat->AppendResult(pCombat->m_result_obj);

    InsertSaveCombat(pCombat);
    return HC_SUCCESS;
}

void zstMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    if (pc->m_generalSoulOpen)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_zst) );
        obj.push_back( Pair("active", 0) );
        int challenge_times = pc->queryExtraData(char_data_type_daily, char_data_daily_zst_challenge);
        int buy_times = pc->queryExtraData(char_data_type_daily, char_data_daily_zst_buy_challenge);
        int left = iZSTChallengeTimes + buy_times - challenge_times;
        boost::shared_ptr<char_zst> cz = getChar(pc->m_id);
        if (cz.get() && cz->m_cur_map > 0 && cz->m_cur_stage > 0)
        {
            ++left;
        }
        obj.push_back( Pair("leftNums", left) );
        blist.push_back(obj);
    }
}

void zstMgr::resetAll()
{
    std::map<int, boost::shared_ptr<char_zst> >::iterator it = m_char_datas.begin();
    while (it != m_char_datas.end())
    {
        if(it->second.get())
        {
            it->second->Clear();
        }
        ++it;
    }
}

//查询战神台地图界面 cmd ：queryZstMapInfo
int ProcessQueryZstMapInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().queryZstMapInfo(*pc, o, robj);
}

//查询战神台场景界面 cmd ：queryZstStageInfo
int ProcessQueryZstStageInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().queryZstStageInfo(*pc, o, robj);
}

//刷新战神台星级cmd：refreshZstStar
int ProcessRefreshZstStar(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().refreshZstStar(*pc, o, robj);
}

//购买战神台挑战次数 cmd：buyZstChallenge
int ProcessBuyZstChallenge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().buyZstChallenge(*pc, robj);
}

//领取战神台星级奖励 cmd：getZstStarReward
int ProcessGetZstStarReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().getZstStarReward(*pc, robj);
}

//查询战神台星级奖励 cmd：queryZstStarReward
int ProcessQueryZstStarReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().queryZstStarReward(*pc, robj);
}

//领取战神台地图奖励 cmd：getZstMapReward
int ProcessGetZstMapReward(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().getZstMapReward(*pc, o, robj);
}

//挑战战神台 cmd：ZstChallenge
int ProcessZstChallenge(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    return Singleton<zstMgr>::Instance().ZstChallenge(*pc, o, robj);
}

