
#include "explore.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "utils_lang.h"
#include "statistics.h"

#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>

Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern std::string strExploreGoldMsg;

exploreMgr* exploreMgr::m_handle = NULL;

exploreMgr* exploreMgr::getInstance()
{
    if (NULL == m_handle)
    {
        m_handle = new exploreMgr();
        m_handle->reload();
    }
    return m_handle;
}

int exploreMgr::reload()
{
    Query q(GetDb());
    q.get_result("SELECT id,spic,name,skill_id,silver_fac,needlevel FROM base_explore_place WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<explore_place> ep;
        ep.reset(new explore_place);
        ep->id = q.getval();
        ep->spic = q.getval();
        ep->name = q.getstr();
        ep->skill_id = q.getval();
        ep->silver_fac = q.getnum();
        ep->needlevel = q.getval();
        m_base_explore_list[ep->id] = ep;
    }
    q.free_result();
    //cout << "*********************load base_explore_place" << endl;
    q.get_result("SELECT id,type,mapid,need,combo1,combo2,combo3 FROM base_explore_combo_reward WHERE 1");
    CHECK_DB_ERR(q);
    while(q.fetch_row())
    {
        boost::shared_ptr<explore_reward> er;
        er.reset(new explore_reward);
        er->id = q.getval();
        er->type = q.getval();
        er->mapid = q.getval();
        er->need = q.getval();
        int tmp = q.getval();
        if (tmp > 0)
        {
            er->combo.push_back(tmp);
        }
        tmp = q.getval();
        if (tmp > 0)
        {
            er->combo.push_back(tmp);
        }
        tmp = q.getval();
        if (tmp > 0)
        {
            er->combo.push_back(tmp);
        }
        m_base_explore_reward.push_back(er);
    }
    q.free_result();
    //cout << "*********************load base_explore_combo_reward" << endl;
    return 0;
}

int exploreMgr::Save(int cid)
{
    std::map<int, boost::shared_ptr<exploreplace_list> >::iterator it = m_char_can_explore_list.find(cid);
    if (it != m_char_can_explore_list.end())
    {
        if (it->second.get())
        {
            std::string sqlcmd = "update char_explore_can set";
            std::list<int>::iterator it_i = it->second->begin();
            size_t i = 1;
            while (it_i != it->second->end())
            {
                if (i == it->second->size())
                {
                    sqlcmd += " place" + LEX_CAST_STR(i) + "=" + LEX_CAST_STR(*it_i);
                }
                else
                {
                    sqlcmd += " place" + LEX_CAST_STR(i) + "=" + LEX_CAST_STR(*it_i) + ",";
                }
                ++it_i;
                ++i;
            }
            if (i > 1)
            {
                sqlcmd += " where cid=" + LEX_CAST_STR(it->first);
                InsertSaveDb(sqlcmd);
            }
        }
    }
    it = m_char_has_explore_list.find(cid);
    if (it != m_char_has_explore_list.end())
    {
        if (it->second.get())
        {
            std::string sqlcmd = "update char_explore_has set";
            std::list<int>::iterator it_i = it->second->begin();
            size_t i = 1;
            while (it_i != it->second->end())
            {
                if (i == it->second->size())
                {
                    sqlcmd += " place" + LEX_CAST_STR(i) + "=" + LEX_CAST_STR(*it_i);
                }
                else
                {
                    sqlcmd += " place" + LEX_CAST_STR(i) + "=" + LEX_CAST_STR(*it_i) + ",";
                }
                ++it_i;
                ++i;
            }
            if (i > 1)
            {
                sqlcmd += " where cid=" + LEX_CAST_STR(it->first);
                InsertSaveDb(sqlcmd);
            }
        }
    }
    return 0;
}

int exploreMgr::Explore(int cid, int pid, json_spirit::Object& robj)
{
    json_spirit::Object getobj;
    json_spirit::Object combobj;
    boost::shared_ptr<exploreplace_list> can_list = exploreMgr::getInstance()->GetCanExplore(cid);
    if (!can_list.get())
    {
        cout << "can_list get fail" << endl;
        return HC_ERROR;
    }
    bool has_place = false;
    exploreplace_list::iterator ite = (*can_list).begin();
    while (ite != (*can_list).end())
    {
        if ((*ite) == pid)
        {
            has_place = true;
            break;
        }
        ++ite;
    }
    if (!has_place)
        return HC_ERROR;
    boost::shared_ptr<CharData> p_cd = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (p_cd.get())
    {
        if (p_cd->m_first_explore == 0)
        {
            p_cd->m_first_explore = 1;
        }
        else if (p_cd->m_second_explore == 0)
        {
            p_cd->m_second_explore = 1;
        }
        //单个奖励
        std::map<int, boost::shared_ptr<explore_place> >::iterator it = m_base_explore_list.find(pid);
        if (it == m_base_explore_list.end())
        {
            return HC_ERROR;
        }

        //判断令
        //if (p_cd->addExploreLing(-1) < 0)
        if (p_cd->addExploreLing(-1) < 0)
        {
            if (p_cd->addLing(-1) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_LING;
            }
        }
        //探索获得军粮
        int supply = 10;
        p_cd->addTreasure(treasure_type_supply, supply);
#if 1    //银币产出修改
        int silver = 50 * p_cd->m_level;
        //银币获得统计
        add_statistics_of_silver_get(p_cd->m_id,p_cd->m_ip_address,silver,silver_get_explore, p_cd->m_union_id, p_cd->m_server_id);
        getobj.push_back( Pair("silver", silver));
        getobj.push_back( Pair("fac", 1));
        
        robj.push_back( Pair("get", getobj));

        if (pid != tianshuge)
        {
            //combo奖励
            bool getresult = false;
            boost::shared_ptr<explore_reward> reward_result;
            reward_result.reset();
            for (int i = 2; i < 5; ++i)
            {
                reward_result = CheckCombo(cid, pid, i);
                if (reward_result.get())
                {
                    getresult = true;
                    break;
                }
            }
            //获得奖励
            if (getresult)
            {
                if (reward_result->type == 1)//skill
                {
                    int level = p_cd->skillLevelup(reward_result->id, p_cd->m_level / 2);
                    if (level > 0)
                    {
                        boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(reward_result->id);
                        if (bs.get())
                        {
                            json_spirit::Object skill;
                            skill.push_back( Pair("id", reward_result->id));
                            skill.push_back( Pair("name", bs->name));
                            skill.push_back( Pair("level", level));
                            skill.push_back( Pair("spic", reward_result->id));
                            combobj.push_back( Pair("skill", skill));
                        }
                    }
                    else
                    {
                        boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(reward_result->id);
                        if (bs.get())
                        {
                            json_spirit::Object skill;
                            skill.push_back( Pair("id", reward_result->id));
                            skill.push_back( Pair("name", bs->name));
                            skill.push_back( Pair("level", level));
                            skill.push_back( Pair("spic", reward_result->id));
                            combobj.push_back( Pair("skill", skill));
                        }
                        robj.push_back( Pair("combo", combobj));
                        return HC_ERROR_SKILL_LEVEL_MAX;
                    }
                }
                else if(reward_result->type == 2)//general_treasure
                {
                    json_spirit::Object gem;
                    std::string general_name = "";
                    std::string baowu_name = "";
                    int gid = p_cd->m_generals.GetGeneralByType(reward_result->id);
                    if (HC_SUCCESS == p_cd->m_generals.UpdateTreasure(general_name, baowu_name, gid))
                    {
                        std::string msg = strExploreGetTreasure;
                        str_replace(msg, "$G", general_name);
                        str_replace(msg, "$T", baowu_name);
                        gem.push_back( Pair("msg", msg));
                        gem.push_back( Pair("name", baowu_name));
                        gem.push_back( Pair("spic", 8));
                    }
                    else
                    {
                        return HC_ERROR_GENERALTREASURE_MAX;
                    }
                    combobj.push_back( Pair("gem", gem) );
                }
            }
            else
            {
                boost::shared_ptr<exploreplace_list> has_list = GetHasExplore(cid);
                (*has_list).push_back(pid);
                if ((*has_list).size() > 4)
                {
                    //cout << "has_list > 4:erase" << (*(*has_list).begin()) << endl;
                    (*has_list).erase((*has_list).begin());
                }
            }
            robj.push_back( Pair("combo", combobj));

            //统计次数，根据引导统计4次
            int explore_num = p_cd->queryExtraData(char_data_type_normal, char_data_explore_num);
            if (explore_num < 4)
            {
                p_cd->setExtraData(char_data_type_normal, char_data_explore_num, explore_num + 1);
            }
        }
        p_cd->NotifyCharData();
        return HC_SUCCESS;
#else
        boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
        int result = 0;//silver1,gold2,ling3,ore4取消令掉落12-3-23
        if (pid < tianshuge)
        {
            if (p_cd->m_level < it->second->needlevel)
            {
                return HC_ERROR;
            }
            //矿石概率20%
            //银币概率79%
            //金币概率1%
            int prob[] = {20,79,1};
            int pmap[] = {4,1,2};
            boost::random::discrete_distribution<> dist(prob);
            result = pmap[dist(gen)];
            switch(result)
            {
                case 2://gold
                    {
                        //70%(5个金币)，19%(10金币),10%(20金币),1%(100金币)
                        int nummap[] = {70,19,10,1};
                        int numprob[] = {5,10,20,100};
                        boost::random::discrete_distribution<> distg(nummap);
                        int num = numprob[distg(gen)];
                        if (num == 100)
                        {
                            std::string msg = strExploreGoldMsg;
                            str_replace(msg, "$G", LEX_CAST_STR(100));
                            str_replace(msg, "$W", p_cd->m_name);
                            GeneralDataMgr::getInstance()->broadCastSysMsg(msg,-1);
                        }
                        p_cd->addGold(num);
                        //金币获得统计
                        add_statistics_of_gold_get(p_cd->m_id,p_cd->m_ip_address,num,gold_get_explore);
                        getobj.push_back( Pair("gold", num));
                        getobj.push_back( Pair("fac", num/5));
                    }
                    break;
                case 1://silver
                default:
                    {
                        //银币 1倍地图20%, 2倍地图50%，4倍地图30%
                        int nummap[] = {20,50,30};
                        int numprob[] = {1,2,4};
                        boost::random::discrete_distribution<> distg(nummap);
                        int fac = numprob[distg(gen)];
                        int getsilver = 0;
                        if (p_cd->m_area >= 1)
                        {
                            getsilver = (int)(30.0 * (double)p_cd->m_area * (double)p_cd->m_level * it->second->silver_fac);
                        }
                        getsilver *= fac;
                        //官职技能市易法
                        int skill_add_per = 0;
                        p_cd->getOfficalSkillLevel(4,skill_add_per);//官职4号技能
                        getsilver = (100 + skill_add_per) * getsilver / 100;
                        p_cd->addSilver(getsilver);
                        //银币获得统计
                        add_statistics_of_silver_get(p_cd->m_id,p_cd->m_ip_address,getsilver,silver_get_explore);
                        getobj.push_back( Pair("silver", getsilver));
                        getobj.push_back( Pair("fac", fac));
                    }
                    break;
            }

            getobj.push_back( Pair("supply", supply) );

            robj.push_back( Pair("get", getobj));
            
            //combo奖励
            bool getresult = false;
            boost::shared_ptr<explore_reward> reward_result;
            reward_result.reset();
            for (int i = 2; i < 5; ++i)
            {
                reward_result = CheckCombo(cid, pid, i);
                if (reward_result.get())
                {
                    getresult = true;
                    break;
                }
            }
            //获得奖励
            if (getresult)
            {
                if (reward_result->type == 1)//skill
                {
                    int level = p_cd->skillLevelup(reward_result->id, p_cd->m_level / 2);
                    if (level > 0)
                    {
                        boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(reward_result->id);
                        if (bs.get())
                        {
                            json_spirit::Object skill;
                            skill.push_back( Pair("id", reward_result->id));
                            skill.push_back( Pair("name", bs->name));
                            skill.push_back( Pair("level", level));
                            skill.push_back( Pair("spic", reward_result->id));
                            combobj.push_back( Pair("skill", skill));
                        }
                    }
                    else
                    {
                        boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(reward_result->id);
                        if (bs.get())
                        {
                            json_spirit::Object skill;
                            skill.push_back( Pair("id", reward_result->id));
                            skill.push_back( Pair("name", bs->name));
                            skill.push_back( Pair("level", level));
                            skill.push_back( Pair("spic", reward_result->id));
                            combobj.push_back( Pair("skill", skill));
                        }
                        robj.push_back( Pair("combo", combobj));
                        return HC_ERROR_SKILL_LEVEL_MAX;
                    }
                }
                else if(reward_result->type == 2)//general_treasure
                {
                    json_spirit::Object gem;
                    std::string general_name = "";
                    std::string baowu_name = "";
                    int gid = p_cd->m_generals.GetGeneralByType(reward_result->id);
                    if (HC_SUCCESS == p_cd->m_generals.UpdateTreasure(general_name, baowu_name, gid))
                    {
                        std::string msg = strExploreGetTreasure;
                        str_replace(msg, "$G", general_name);
                        str_replace(msg, "$T", baowu_name);
                        gem.push_back( Pair("msg", msg));
                        gem.push_back( Pair("name", baowu_name));
                        gem.push_back( Pair("spic", 8));
                    }
                    else
                    {
                        return HC_ERROR_GENERALTREASURE_MAX;
                    }
                    combobj.push_back( Pair("gem", gem) );
                }
            }
            else
            {
                boost::shared_ptr<exploreplace_list> has_list = GetHasExplore(cid);
                (*has_list).push_back(pid);
                if ((*has_list).size() > 4)
                {
                    //cout << "has_list > 4:erase" << (*(*has_list).begin()) << endl;
                    (*has_list).erase((*has_list).begin());
                }
            }
            robj.push_back( Pair("combo", combobj));

            //统计次数，根据引导统计4次
            int explore_num = p_cd->queryExtraData(char_data_type_normal, char_data_explore_num);
            if (explore_num < 4)
            {
                p_cd->setExtraData(char_data_type_normal, char_data_explore_num, explore_num + 1);
            }
            return HC_SUCCESS;
        }
        else// if (pid == tianshuge)//天书阁
        {
            if (p_cd->m_level < it->second->needlevel)
            {
                return HC_ERROR;
            }
            
            //洗髓经概率10%
            //银币概率80%
            //金币概率10%
            int prob[] = {10,80,10};
            int pmap[] = {4,1,2};
            boost::random::discrete_distribution<> dist(prob);
            result = pmap[dist(gen)];
            switch (result)
            {
                case 2://gold
                    {
                        //70%(5个金币)，20%(10金币),10%(20金币)
                        int nummap[] = {70,20,10};
                        int numprob[] = {5,10,20};
                        boost::random::discrete_distribution<> distg(nummap);
                        int num = numprob[distg(gen)];
                        p_cd->addGold(num);
                        //金币获得统计
                        add_statistics_of_gold_get(p_cd->m_id,p_cd->m_ip_address,num,gold_get_explore);
                        getobj.push_back( Pair("gold", num));
                        getobj.push_back( Pair("fac", num/5));
                    }
                    break;
                case 4://洗髓
                    {
                        json_spirit::Object gem;
                        int xisuijing = 7;//洗髓经宝物ID
                        gem.push_back( Pair("id", xisuijing));
                        boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(xisuijing);
                        if (tr.get())
                        {
                            gem.push_back( Pair("name", tr->name));
                            gem.push_back( Pair("spic", tr->spic));
                            gem.push_back( Pair("quality", tr->quality));
                            p_cd->addTreasure(xisuijing, 1);
                        }
                        else
                        {
                            ERR();
                        }
                        getobj.push_back( Pair("gem", gem) );
                    }
                    break;
                case 1://silver
                default:
                    {
                        //银币 1倍地图20%, 2倍地图50%，4倍地图30%
                        int nummap[] = {20,50,30};
                        int numprob[] = {1,2,4};
                        boost::random::discrete_distribution<> distg(nummap);
                        int fac = numprob[distg(gen)];

                        int getsilver = 0;
                        if (p_cd->m_area >= 1)
                        {
                            getsilver = (int)(30.0 * (double)p_cd->m_area * (double)p_cd->m_level * it->second->silver_fac);
                        }
                        getsilver *= fac;
                        //官职技能市易法
                        int skill_add_per = 0;
                        p_cd->getOfficalSkillLevel(4,skill_add_per);//官职4号技能
                        getsilver = (100 + skill_add_per) * getsilver / 100;
                        p_cd->addSilver(getsilver);
                        //银币获得统计
                        add_statistics_of_silver_get(p_cd->m_id,p_cd->m_ip_address,getsilver,silver_get_explore);
                        getobj.push_back( Pair("silver", getsilver));
                        getobj.push_back( Pair("fac", fac));
                    }
                    break;
            }

            getobj.push_back( Pair("supply", supply) );

            robj.push_back( Pair("get", getobj));

            //统计次数，根据引导统计4次
            int explore_num = p_cd->queryExtraData(char_data_type_normal, char_data_explore_num);
            if (explore_num < 4)
            {
                p_cd->setExtraData(char_data_type_normal, char_data_explore_num, explore_num + 1);
            }
            return HC_SUCCESS;
        }
#endif
    }
    
    return HC_ERROR_LOGIN_FIRST;
}

//购买探索次数
int exploreMgr::buyExploreLing(CharData* pc, json_spirit::Object& robj)
{
    if (!pc)
    {
        return HC_ERROR;
    }
    //购买次数最大了
    int buy_time = pc->queryExtraData(char_data_type_daily, char_data_buy_explore) + 1;

    //金币够吗
    int gold_cost = 0;
    for (int i = 0; i < iExploreCostLevel; ++i)
    {
        if (buy_time <= iExploreBuyGold[i][0])
        {
            gold_cost = iExploreBuyGold[i][1];
            break;
        }
    }
    if (gold_cost > iExploreBuyGoldMax)
    {
        gold_cost = iExploreBuyGoldMax;
    }
    if (pc->addGold(-gold_cost) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    //金币消耗统计
    add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, gold_cost, gold_cost_for_buy_explore, pc->m_union_id, pc->m_server_id);

    //保存购买次数
    pc->setExtraData(char_data_type_daily, char_data_buy_explore, buy_time);

    pc->addExploreLing(1);
    robj.push_back( Pair("ling", pc->exploreLing()) );

    if (gold_cost < iExploreBuyGoldMax)
    {
        ++buy_time;
    }
    for (int i = 0; i < iExploreCostLevel; ++i)
    {
        if (buy_time <= iExploreBuyGold[i][0])
        {
            gold_cost = iExploreBuyGold[i][1];
            break;
        }
    }
    if (gold_cost > iExploreBuyGoldMax)
    {
        gold_cost = iExploreBuyGoldMax;
    }
    robj.push_back( Pair("buyGold", gold_cost));
    return HC_SUCCESS;
}

int exploreMgr::ExploreRefresh(int cid, bool cost_gold)
{
    boost::shared_ptr<CharData> p_cd = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!p_cd.get())
    {
        return HC_ERROR;
    }
    if (!p_cd->m_exploreOpen)
    {
        return HC_ERROR;
    }
    if (cost_gold && p_cd->m_vip < iExploreRefreshVIP)
    {
        return HC_ERROR_MORE_VIP_LEVEL;
    }
    int place_num = p_cd->m_vip >= 3 ? 4 : 3;
    int explore_num = p_cd->queryExtraData(char_data_type_normal, char_data_explore_num);
    if (explore_num == 0)//第一次访问刷广场在第一个位置配合引导
    {
        boost::shared_ptr<exploreplace_list> n_list;
        n_list.reset(new exploreplace_list);
        for(int i = 0; i < place_num; ++i)
        {
            if (i == 0)
            {
                n_list->push_back(3);
            }
            else if (i == 1)
            {
                n_list->push_back(1);
            }
            else if (i == 2)
            {
                n_list->push_back(2);
            }
            else
            {
                n_list->push_back(4);
            }
        }
        m_char_can_explore_list[cid] = n_list;
        return HC_SUCCESS;
    }
    else if (explore_num == 1)//第二次访问刷军寨在第一个位置配合引导
    {
        boost::shared_ptr<exploreplace_list> n_list;
        n_list.reset(new exploreplace_list);
        for(int i = 0; i < place_num; ++i)
        {
            if (i == 0)
            {
                n_list->push_back(2);
            }
            else if (i == 1)
            {
                n_list->push_back(1);
            }
            else if (i == 2)
            {
                n_list->push_back(3);
            }
            else
            {
                n_list->push_back(4);
            }
        }
        m_char_can_explore_list[cid] = n_list;
        return HC_SUCCESS;
    }
    else if (explore_num == 2)//第三次访问刷庙宇在第一个位置配合引导
    {
        boost::shared_ptr<exploreplace_list> n_list;
        n_list.reset(new exploreplace_list);
        for(int i = 0; i < place_num; ++i)
        {
            if (i == 0)
            {
                n_list->push_back(1);
            }
            else if (i == 1)
            {
                n_list->push_back(2);
            }
            else if (i == 2)
            {
                n_list->push_back(3);
            }
            else
            {
                n_list->push_back(4);
            }
        }
        m_char_can_explore_list[cid] = n_list;
        return HC_SUCCESS;
    }
    else if (explore_num == 3)//第四次访问刷客栈在第一个位置配合引导
    {
        boost::shared_ptr<exploreplace_list> n_list;
        n_list.reset(new exploreplace_list);
        for(int i = 0; i < place_num; ++i)
        {
            if (i == 0)
            {
                n_list->push_back(4);
            }
            else if (i == 1)
            {
                n_list->push_back(1);
            }
            else if (i == 2)
            {
                n_list->push_back(2);
            }
            else
            {
                n_list->push_back(3);
            }
        }
        m_char_can_explore_list[cid] = n_list;
        return HC_SUCCESS;
    }
    int max_placeid = 10;
    if (cost_gold)
    {
        if (p_cd->m_vip < iExploreRefreshUnlimitVIP && p_cd->m_explore_refresh_times > iExploreRefreshTimes[p_cd->m_vip])
        {
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        int gold_need = GetGoldRefreshGold(cid);
        if (-1 == p_cd->addGold(-gold_need))
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        add_statistics_of_gold_cost(p_cd->m_id, p_cd->m_ip_address, gold_need, gold_cost_for_refresh_explore, p_cd->m_union_id, p_cd->m_server_id);
        p_cd->NotifyCharData();
        ++p_cd->m_explore_refresh_times;
        p_cd->saveCharDailyVar();
        boost::shared_ptr<exploreplace_list> list = GetCanExplore(cid);
        boost::shared_ptr<exploreplace_list> n_list;
        int get_e = false;

        //if (p_cd->m_level < iExploreBaseOpenLevel)
        //{
        //    max_placeid = 8;
        //}
        //else if (p_cd->m_level < iExploreTianOpenLevel)
        //{
        //    max_placeid = 10;
        //}
        //else
        //{
        //    //天书阁
        //    //if (my_random(0,100) < 5)
        //    //{
        //    //    get_e = true;
        //    //    place_num--;
        //    //}
        //    max_placeid = 10;
        //}
        n_list.reset(new exploreplace_list);
        for (int i = 0; i < place_num;)
        {
            int rand = my_random(1,max_placeid);
            if (!CheckPlaceGetAlready(rand, list) && !CheckPlaceGetAlready(rand, n_list))
            {
                n_list->push_back(rand);
                ++i;
            }
        }
        //插入到随机位置
        if (get_e)
        {
            int rand = my_random(0,(place_num));
            std::list<int>::iterator tmp_it = n_list->begin();
            while(tmp_it != n_list->end() && rand != 0)
            {
                --rand;
                ++tmp_it;
            }
            n_list->insert(tmp_it, tianshuge);
        }
        list->clear();
        //非V最后一个未开放
        if (n_list->size() == 3)
        {
            n_list->push_back(-1);
        }
        m_char_can_explore_list[cid] = n_list;
    }
    else
    {
        boost::shared_ptr<exploreplace_list> list = GetCanExplore(cid);
        boost::shared_ptr<exploreplace_list> n_list;
        int get_e = false;
        //if(p_cd->m_level < iExploreBaseOpenLevel)
        //{
        //    max_placeid = 8;
        //}
        //else if(p_cd->m_level < iExploreTianOpenLevel)
        //{
        //    max_placeid = 10;
        //}
        //else
        //{
        //    //天书阁
        //    //if (my_random(0,100) < 5)
        //    //{
        //    //    get_e = true;
        //    //    place_num--;
        //    //}
        //    max_placeid = 10;
        //}
        n_list.reset(new exploreplace_list);
        for(int i = 0; i < place_num;)
        {
            int rand = my_random(1,max_placeid);
            //cout << "rand=" << rand << endl;
            if (!CheckPlaceGetAlready(rand, n_list))
            {
                //cout << "***********push rand=" << rand << endl;
                n_list->push_back(rand);
                ++i;
            }
        }
        //插入到随机位置
        if (get_e)
        {
            int rand = my_random(0,(place_num - 1));
            std::list<int>::iterator tmp_it = n_list->begin();
            while(tmp_it != n_list->end() && rand != 0)
            {
                --rand;
                ++tmp_it;
            }
            n_list->insert(tmp_it, tianshuge);
        }
        list->clear();
        //非V最后一个未开放
        if (n_list->size() == 3)
        {
            n_list->push_back(-1);
        }
        m_char_can_explore_list[cid] = n_list;
    }
    return HC_SUCCESS;
}

bool exploreMgr::CheckPlaceGetAlready(int pid, boost::shared_ptr<exploreplace_list> p_list)
{
    //cout << "check already is call" << endl;
    std::list<int>::iterator it = p_list->begin();
    while (it != p_list->end())
    {
        if (pid == *it)
        {
            return true;
        }
        ++it;
    }
    return false;
}

int exploreMgr::GetGoldRefreshGold(int cid)
{
    boost::shared_ptr<CharData> p_cd = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!p_cd.get())
    {
        return 0;
    }
    if (p_cd->m_explore_refresh_times < 0)
    {
        p_cd->m_explore_refresh_times = 0;
        p_cd->saveCharDailyVar();
    }
    int ret = p_cd->m_explore_refresh_times * iExploreRefreshGold_Add + iExploreRefreshGold_First;
    if (ret >= iExploreRefreshGold_Max)
    {
        ret = iExploreRefreshGold_Max;
    }
    else if (ret < 0)
    {
        ret = iExploreRefreshGold_Max;
    }
    return ret;
}

int exploreMgr::GetComboPos(int cid, int pid)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
    {
        return 0;
    }
    int MaxComboPos = 0;
    boost::shared_ptr<exploreplace_list> has_list = exploreMgr::getInstance()->GetHasExplore(cdata->m_id);
    if (!has_list.get())
    {
        return 0;
    }
    std::list<boost::shared_ptr<explore_reward> > er_list;
    er_list = GetExploreReward();
    if (er_list.empty())
    {
        return 0;
    }
    std::list<boost::shared_ptr<explore_reward> >::iterator it = er_list.begin();
    while (it != er_list.end())
    {
        if ((*it).get() && (*it)->mapid <= cdata->m_area)
        {
            //技能如果等级不到则直接跳过
            if ((*it)->type == 1 && (*it)->need > cdata->m_level)
            {
                ++it;
                continue;
            }
            //宝物如果季节不对或者没有该武将则直接跳过
            if ((*it)->type == 2 && ((*it)->need != GeneralDataMgr::getInstance()->getSeason() || !(cdata->CheckHasGeneral((*it)->id))))
            {
                ++it;
                continue;
            }
            int cnt = 0;
            for (size_t i = 1; i <= (*it)->combo.size(); ++i)
            {
                int N = (*it)->combo.size() - i;//比较长度从长到短
                std::list<int> tmp;
                tmp.push_front(pid);//把鼠标指向的pid加入到已访问末尾进行比较
                exploreplace_list::reverse_iterator ite = (*has_list).rbegin();
                while (N != 0 && ite != (*has_list).rend())
                {
                    tmp.push_front(*ite);
                    --N;
                    ++ite;
                }
                if (N != 0)//has_list不够长无法匹配，继续下个长度
                {
                    continue;
                }
                else
                {
                    std::list<int>::iterator it_tmp = tmp.begin();
                    size_t i_combo = 0;
                    for( ; it_tmp != tmp.end(),i_combo < (*it)->combo.size(); ++it_tmp, ++i_combo)
                    {
                        if((*it)->combo[i_combo] == *it_tmp)//匹配到则计数器增加
                        {
                            ++cnt;
                        }
                        else if(it_tmp != tmp.end())//没匹配完tmp就结束则属无效
                        {
                            cnt = 0;
                            break;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (cnt != 0)//匹配成功跳出循环
                    {
                        break;
                    }
                }
            }
            if (cnt != 0 && cnt > MaxComboPos)
            {
                MaxComboPos = cnt;
            }
        }
        ++it;
    }
    return MaxComboPos;
}

boost::shared_ptr<explore_reward> exploreMgr::CheckCombo(int cid, int pid, int num)
{
    boost::shared_ptr<CharData> p_cd = GeneralDataMgr::getInstance()->GetCharData(cid);
    std::list<int> tmp;
    tmp.push_front(pid);
    boost::shared_ptr<exploreplace_list> has_list = GetHasExplore(cid);
    for (int i = 0; i < num - 1; ++i)
    {
        //数量不够，装回数据返回空值
        if ((*has_list).empty())
        {
            for (std::list<int>::iterator it = tmp.begin(); it != tmp.end(); ++it)
            {
                (*has_list).push_back(*it);
            }
            (*has_list).pop_back();
            boost::shared_ptr<explore_reward> pt;
            pt.reset();
            return pt;
        }
        tmp.push_front((*has_list).back());
        (*has_list).pop_back();
    }
    if (p_cd.get())
    {
        std::list<boost::shared_ptr<explore_reward> >::iterator it = m_base_explore_reward.begin();
        while(it != m_base_explore_reward.end())
        {
            if ((int)(*it)->combo.size() == num && (*it)->mapid <= p_cd->m_area)
            {
                //技能如果等级不到则直接跳过
                if ((*it)->type == 1 && (*it)->need > p_cd->m_level)
                {
                    ++it;
                    continue;
                }
                //宝物如果季节不对或者没有该武将则直接跳过
                if ((*it)->type == 2 && ((*it)->need != GeneralDataMgr::getInstance()->getSeason() || !(p_cd->CheckHasGeneral((*it)->id))))
                {
                    ++it;
                    continue;
                }
                int i = 0;
                bool combo_success = true;
                for (std::list<int>::iterator it_tmp = tmp.begin(); it_tmp != tmp.end(); ++it_tmp, ++i)
                {
                    if (*it_tmp != (*it)->combo[i])
                    {
                        combo_success = false;
                        break;
                    }
                }
                if (combo_success)
                {
                    return *it;
                }
            }
            ++it;
        }
    }
    else
    {
        ERR();
    }
    //匹配不到数据装回，最末的pid不插入
    for (std::list<int>::iterator it = tmp.begin(); it != tmp.end(); ++it)
    {
        (*has_list).push_back(*it);
    }
    (*has_list).pop_back();
    boost::shared_ptr<explore_reward> pt;
    pt.reset();
    return pt;
}

std::string exploreMgr::GetRewardName(int type,int id)
{
    Query q(GetDb());
    std::string cname = "";
    if (type == 1)
    {
        boost::shared_ptr<baseSkill> bskill = baseSkillMgr::getInstance()->GetBaseSkill(id);
        if (bskill.get())
        {
            cname = bskill->name;
        }
    }
    else if (type == 2)
    {
        boost::shared_ptr<GeneralTypeData> gd = GeneralDataMgr::getInstance()->GetBaseGeneral(id);
        if (gd.get())
        {
            cname = gd->m_baowu;
        }
    }
    return cname;
}

std::string exploreMgr::GetPlaceName(int pid)
{
    std::string cname = "";
    std::map<int, boost::shared_ptr<explore_place> >::iterator it = m_base_explore_list.find(pid);
    if (it != m_base_explore_list.end())
    {
        if (it->second.get())
        {
            cname = it->second->name;
        }
    }
    return cname;
}

std::list<boost::shared_ptr<explore_reward> > exploreMgr::GetExploreReward()
{
    return m_base_explore_reward;
}

boost::shared_ptr<explore_place> exploreMgr::GetBaseExplorePlace(int pid)
{
    std::map<int, boost::shared_ptr<explore_place> >::iterator it = m_base_explore_list.find(pid);
    if (it != m_base_explore_list.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<explore_place> p;
        p.reset();
        return p;
    }
}

boost::shared_ptr<exploreplace_list> exploreMgr::GetHasExplore(int cid)
{
    std::map<int, boost::shared_ptr<exploreplace_list> >::iterator it = m_char_has_explore_list.find(cid);
    if (it != m_char_has_explore_list.end())
    {
        return it->second;
    }
    else
    {
        Query q(GetDb());
        boost::shared_ptr<exploreplace_list> ep_list;
        ep_list.reset(new exploreplace_list);
        q.get_result("select place1,place2,place3,place4 from char_explore_has where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            int tmp = q.getval();
            if (tmp > 0)
            {
                ep_list->push_back(tmp);
            }
            tmp = q.getval();
            if (tmp > 0)
            {
                ep_list->push_back(tmp);
            }
            tmp = q.getval();
            if (tmp > 0)
            {
                ep_list->push_back(tmp);
            }
            tmp = q.getval();
            if (tmp > 0)
            {
                ep_list->push_back(tmp);
            }
            q.free_result();
        }
        else
        {
            q.free_result();
            if (!q.execute("insert into char_explore_has set cid=" + LEX_CAST_STR(cid)))
            {
                DB_ERROR(q);
            }
        }
        m_char_has_explore_list[cid] = ep_list;
        return ep_list;
    }
}

boost::shared_ptr<exploreplace_list> exploreMgr::GetCanExplore(int cid)
{
    std::map<int, boost::shared_ptr<exploreplace_list> >::iterator it = m_char_can_explore_list.find(cid);
    if (it != m_char_can_explore_list.end())
    {
        return it->second;
    }
    else
    {
        Query q(GetDb());
        boost::shared_ptr<exploreplace_list> ep_list;
        ep_list.reset(new exploreplace_list);
        q.get_result("select place1,place2,place3,place4 from char_explore_can where cid=" + LEX_CAST_STR(cid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            ep_list->push_back(q.getval());
            ep_list->push_back(q.getval());
            ep_list->push_back(q.getval());
            ep_list->push_back(q.getval());
        }
        q.free_result();
        m_char_can_explore_list[cid] = ep_list;
        return ep_list;
    }
}

int exploreMgr::deleteChar(int cid)
{
    InsertSaveDb("delete from char_explore_can where cid=" + LEX_CAST_STR(cid));
    InsertSaveDb("delete from char_explore_has where cid=" + LEX_CAST_STR(cid));

    m_char_has_explore_list.erase(cid);        //玩家已探索列表
    m_char_can_explore_list.erase(cid);        //玩家可探索列表
    return HC_SUCCESS;
}

