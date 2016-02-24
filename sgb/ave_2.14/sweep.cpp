#include "statistics.h"

#include "sweep.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_timer.h"
#include "spls_const.h"
#include "eliteCombat.h"
#include "SaveDb.h"

class Combat;
extern std::string strGold;
extern std::string strSilver;
extern std::string strLing;
extern std::string strPrestige;
extern std::string strCounts;
extern std::string strSweepMailTitle;
extern std::string strSweepMailContent;

extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool, int statistics_type);
Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern void InsertSaveDb(const saveDbJob& job);
extern int InsertInternalActionWork(json_spirit::mObject& obj);

extern std::string strExp;

//static     boost::uuids::nil_generator gen;

int charSweep::start()
{
    m_sweep_itemlist.clear();
    return _start();
}

int charSweep::_start()
{
    if (m_left_fights <= 0)
    {
        m_start_time = 0;
        m_end_time = 0;
        ERR();
        return HC_ERROR;
    }

    if (m_sweep_task.size() == 0)
    {
        ERR();
        return HC_ERROR;
    }

    if (now_sweep_id == 0)
    {
        now_sweep_id = *(m_sweep_task.begin());
    }

    int leftsecond = 0;
    int fast_finish = 0;
    if (1 == m_fast_mod)
    {
        fast_finish = m_left_fights;
    }
    else
    {
        //重启后的情况
        int now_left_fights = 0;
        leftsecond = m_end_time - time(NULL);
        if (leftsecond > 0)
        {
            now_left_fights = leftsecond / iSweepEliteTime;
            if (leftsecond % iSweepEliteTime != 0)
            {
                ++now_left_fights;
            }
        }
        //直接完成N场
        if (m_left_fights > now_left_fights)
        {
            fast_finish = m_left_fights - now_left_fights;
        }
        //新手没快速完成需要弹窗通知
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
        if (!cdata.get())
            return HC_ERROR;
        if (cdata->isNewPlayer() > 0)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "notify") );
            obj.push_back( Pair("s", 200) );
            obj.push_back( Pair("type", notify_msg_new_player_sweep) );
            obj.push_back( Pair("nums", cdata->isNewPlayer()) );
            cdata->sendObj(obj);
        }
    }
    if (fast_finish > 0)
    {
        json_spirit::mObject mobj;
        mobj["cmd"] = "sweepDone";
        mobj["cid"] = m_cid;
        mobj["total_time"] = fast_finish;
        InsertInternalActionWork(mobj);
        return HC_SUCCESS;
    }
    else
    {
        json_spirit::mObject mobj;
        mobj["cmd"] = "sweepDone";
        mobj["cid"] = m_cid;
        boost::shared_ptr<splsTimer> tmsg;
        mobj["total_time"] = 1;
        int start_first_time = leftsecond % iSweepEliteTime;
        if (start_first_time == 0)
            start_first_time += iSweepEliteTime;
        tmsg.reset(new splsTimer(start_first_time, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    save();
    return HC_SUCCESS;
}

int charSweep::speedup()
{
    if (m_left_fights <= 0)
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
        if (cdata->isNewPlayer() > 0)
            cost_gold = 0;
        if (cost_gold > 0)
        {
            if (cdata->addGold(-cost_gold) == -1)
                return HC_ERROR_NOT_ENOUGH_GOLD;
            //金币消耗统计
            add_statistics_of_gold_cost(cdata->m_id, cdata->m_ip_address, cost_gold, gold_cost_for_sweep, cdata->m_union_id, cdata->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(cdata.get(),cost_gold,gold_cost_for_sweep);
#endif
        }

        if (!_uuid.is_nil())
        {
            splsTimerMgr::getInstance()->delTimer(_uuid);
            _uuid = boost::uuids::nil_uuid();
        }
        m_fast_mod = 1;
        int ret = done_all();
        if (ret != HC_SUCCESS)
        {
            //通知客户端
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
            if (account.get())
            {
                json_spirit::Object obj;
                obj.clear();
                obj.push_back( Pair("cmd", "SweepDone") );
                obj.push_back( Pair("s", ret) );
                account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
            }
        }
        save();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int charSweep::done_all()
{
    return done(m_left_fights);
}

int charSweep::done(int total_time)
{
    if ((0 == m_fast_mod && 0 == m_start_time) || m_left_fights <= 0 || m_sweep_task.size() == 0)
    {
        ERR();
        return HC_ERROR;
    }
    if (total_time <= 0)
    {
        ERR();
        return HC_ERROR;
    }
    if (total_time > m_left_fights)
    {
        total_time = m_left_fights;
    }

    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(m_cid);
    if (!cdata.get())
    {
        ERR();
        return HC_ERROR;
    }
    //cout << "sweep done,left_round=" << m_left_round << " now_sweep_id=" << now_sweep_id << endl;
    std::list<int>::iterator it = m_sweep_task.begin();
    while (it != m_sweep_task.end())
    {
        if (*it != now_sweep_id)
        {
            ++it;
        }
        else
        {            
            break;
        }
    }
    if (it == m_sweep_task.end())
    {
        now_sweep_id = *(m_sweep_task.begin());
    }
    int ling_cnt = 0, need_ling = 0;
    if (m_type == 1)
        need_ling = 4;
    else if(m_type == 2)
        need_ling = 1;
    while (m_left_fights > 0 && total_time > 0)
    {
        if (cdata->addLing(-need_ling) == -1)
        {
            bool need_ret = true;
            //有自动买军令勾选尝试买
            if (m_auto_buy_ling)
            {
                json_spirit::Object robj;
                cdata->rest(2, robj);
                if (cdata->addLing(-need_ling) == -1)
                {
                    need_ret = false;
                }
            }
            
            if (need_ret)
            {
                stop();
                if (ling_cnt > 0)
                    add_statistics_of_ling_cost(cdata->m_id, cdata->m_ip_address, ling_cnt, ling_stronghold, 2, cdata->m_union_id, cdata->m_server_id);
                return HC_ERROR_NOT_ENOUGH_LING;
            }
        }
        ling_cnt += need_ling;
        int loot_mapid = 0, loot_level = 0;
        std::list<Item> getItems;
        if (m_type == 1)//关卡掉落
        {
            boost::shared_ptr<StrongholdData> pshd = GeneralDataMgr::getInstance()->GetStrongholdData(now_sweep_id);
            if (!pshd.get())
                return HC_ERROR;
            lootMgr::getInstance()->getStrongholdLoots(now_sweep_id, getItems, 0);
            if (getItems.size() == 0)
            {
                lootMgr::getInstance()->getWorldItemFall(getItems);
            }
            //加粮草
            Item itm;
            itm.type = item_type_treasure;
            itm.id = treasure_type_supply;
            itm.nums = pshd->m_rob_supply;
            //QQ黄钻掠夺军粮增10%
            if (cdata->m_qq_yellow_level > 0)
            {
                itm.nums = 11 * itm.nums / 10;
            }
            getItems.push_back(itm);
            //功勋奖励
            //int id = (pshd->m_map_id - 1) * 3 + pshd->m_stage_id;
            //int gongxun = iStageGongxun[id - 1];
            Item item_gongxun;
            item_gongxun.type = item_type_treasure;
            item_gongxun.id = treasure_type_gongxun;
            item_gongxun.nums = pshd->m_gongxun;

            getItems.push_back(item_gongxun);
            
            //2.13增加宝箱掉落
            lootMgr::getInstance()->getBoxItemFall(cdata->m_level,getItems);

            if (!m_auto_sell)
            {
                //给东西
                giveLoots(cdata.get(), getItems, pshd->m_map_id, pshd->m_level, pshd->m_isepic, NULL, NULL, true, give_sweep_loot);
            }
            else
            {
                std::list<Item> real_getItems;
                for (std::list<Item>::iterator it_item = getItems.begin(); it_item != getItems.end(); ++it_item)
                {
                    Item& itm = *it_item;
                    switch (itm.type)
                    {
                        case item_type_equipment:
                            {
                                boost::shared_ptr<baseEquipment> be = GeneralDataMgr::getInstance()->GetBaseEquipment(itm.id);
                                if (be.get())
                                {
                                    cdata->addSilver(be->basePrice * itm.nums);
                                }
                            }
                            break;
                        case item_type_treasure:
                            {
                                boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(itm.id);
                                if (bt.get() && (bt->usage == ITEM_USAGE_EQUIPMENT_SCROLL
                                                 || bt->usage == ITEM_USAGE_EQUIPMENT_METRIAL
                                                 || bt->usage == ITEM_USAGE_FOR_TRADE)
                                    && bt->sellPrice)
                                {
                                    cdata->addSilver(bt->sellPrice * itm.nums);
                                }
                                else
                                {
                                    real_getItems.push_back(itm);
                                }
                            }
                            break;
                        default:
                            real_getItems.push_back(itm);
                            break;
                    }
                }

                //给东西
                giveLoots(cdata.get(), real_getItems, pshd->m_map_id, pshd->m_level, pshd->m_isepic, NULL, NULL, true, give_sweep_loot);
            }
            loot_mapid = pshd->m_map_id;
            loot_level = pshd->m_level;

            //掠夺支线任务
            cdata->m_trunk_tasks.updateTask(task_rob_stronghold, pshd->m_id, 1);
            cdata->m_trunk_tasks.updateTask(task_attack_stronghold, pshd->m_id, 1);
        }
        else if (m_type == 2)//精英战役掉落
        {
            boost::shared_ptr<CharEliteCombatData> pced = eliteCombatMgr::getInstance()->getCharEliteCombat(cdata->m_id,m_mapid, now_sweep_id);
            if (!pced.get() || !pced->m_baseEliteCombat.get())
            {
                return HC_ERROR;
            }
            //修改可攻击状态
            pced->m_state = pced->m_result;
            pced->save();
            /***********随机获得掉落处理****************/
            lootMgr::getInstance()->getEliteCombatsLoots(now_sweep_id, getItems, 0);
            if (getItems.size() == 0)
            {
                lootMgr::getInstance()->getWorldItemFall(getItems);
            }
            //军粮
            Item item_sp(item_type_treasure, treasure_type_supply, pced->m_baseEliteCombat->supply, 1);
            getItems.push_back(item_sp);
            //功勋
            Item item_g(item_type_treasure, treasure_type_gongxun, pced->m_baseEliteCombat->gongxun, 1);
            getItems.push_back(item_g);
            if (!m_auto_sell)
            {
                //给东西
                giveLoots(cdata.get(), getItems, m_mapid, pced->m_baseEliteCombat->_level, 0, NULL, NULL, true, give_sweep_loot);
            }
            else
            {
                std::list<Item> real_getItems;
                for (std::list<Item>::iterator it_item = getItems.begin(); it_item != getItems.end(); ++it_item)
                {
                    Item& itm = *it_item;
                    switch (itm.type)
                    {
                        case item_type_equipment:
                            {
                                boost::shared_ptr<baseEquipment> be = GeneralDataMgr::getInstance()->GetBaseEquipment(itm.id);
                                if (be.get())
                                {
                                    cdata->addSilver(be->basePrice * itm.nums);
                                    //银币获得统计
                                    add_statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, be->basePrice * itm.nums, silver_get_sell_equiptment, cdata->m_union_id, cdata->m_server_id);
                                }
                            }
                            break;
                        case item_type_treasure:
                            {
                                boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(itm.id);
                                if (bt.get() && (bt->usage == ITEM_USAGE_EQUIPMENT_SCROLL
                                                 || bt->usage == ITEM_USAGE_EQUIPMENT_METRIAL
                                                 || bt->usage == ITEM_USAGE_FOR_TRADE)
                                    && bt->sellPrice)
                                {
                                    cdata->addSilver(bt->sellPrice * itm.nums);
                                    //银币获得统计
                                    add_statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, bt->sellPrice * itm.nums, silver_get_sell_treasure, cdata->m_union_id, cdata->m_server_id);
                                }
                                else
                                {
                                    real_getItems.push_back(itm);
                                }
                            }
                            break;
                        default:
                            real_getItems.push_back(itm);
                            break;
                    }
                }

                //给东西
                giveLoots(cdata.get(), real_getItems, m_mapid, pced->m_baseEliteCombat->_level, 0, NULL, NULL, true, give_sweep_loot);
            }
            loot_mapid = m_mapid;
            loot_level = pced->m_baseEliteCombat->_level;
        }
        //记录获得的物品
        boost::shared_ptr<SweepResult> psr;
        psr.reset(new SweepResult);
        psr->stronghold_id = now_sweep_id;
        for (std::list<Item>::iterator it_item = getItems.begin(); it_item != getItems.end(); ++it_item)
        {
            //cout << "sweep done a fight:itemid=" << (*it_item).id << ",itemtype=" << (*it_item).type << ",itemnum=" << (*it_item).nums << endl;
            boost::shared_ptr<Item> p;
            p.reset(new Item);
            p->id = (*it_item).id;
            p->type = (*it_item).type;
            if (p->type == item_type_silver_map)
            {
                int silver = getMapSilver(loot_mapid, loot_level, true);
                silver *= (*it_item).nums;
                p->nums = silver;
            }
            else if(p->type == item_type_skill)
            {
                p->nums = cdata->getSkillLevel(p->id);
            }
            else if(p->type == item_type_zhen)
            {
                boost::shared_ptr<ZhenData> zhen = cdata->m_zhens.GetZhen(p->id);
                if (zhen.get())
                    p->nums = zhen->m_level;
            }
            else
                p->nums = (*it_item).nums;

            psr->itemlist.push_back(p);
        }
        m_sweep_itemlist.push_back(psr);
        --m_left_fights;
        --total_time;
        //迭代器后移
        ++it;
        if (it == m_sweep_task.end())//本轮还有
        {
            it = m_sweep_task.begin();
        }
        now_sweep_id = *it;
        //包满停止扫荡
        if (cdata->m_bag.isFull())
        {
            stop();
            if (ling_cnt > 0)
                add_statistics_of_ling_cost(cdata->m_id, cdata->m_ip_address, ling_cnt, (m_type == 2 ? ling_elite_combat : ling_stronghold), 2, cdata->m_union_id, cdata->m_server_id);
            return HC_ERROR_BAG_FULL;
        }
    }
    if (ling_cnt > 0)
        add_statistics_of_ling_cost(cdata->m_id, cdata->m_ip_address, ling_cnt, (m_type == 2 ? ling_elite_combat : ling_stronghold), 2, cdata->m_union_id, cdata->m_server_id);

    if (m_left_fights <= 0)//任务全部结束
    {
        while (m_auto_buy_ling && cdata->ling() < 30)
        {
            json_spirit::Object robj;
            if (HC_SUCCESS != cdata->rest(2, robj))
            {
                break;
            }
        }
        //通知客户端完成一场扫荡
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
        //if (account.get())
        //{
        //    json_spirit::Object obj;
        //    obj.clear();
        //    obj.push_back( Pair("cmd", "SweepDone") );
        //    obj.push_back( Pair("s", 200) );
        //    account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        //}
        //cout << "m_sweep_task is empty" << endl;
        //发送信件
        std::string content = strSweepMailContent;
        int cost_min = (m_end_time - m_start_time) / 60;
        if (cost_min < 0)
            cost_min = (time(NULL) - m_start_time) / 60;
        str_replace(content, "$T", LEX_CAST_STR(cost_min));
        if (m_type == 2)
        {
            str_replace(content, "$F", LEX_CAST_STR(m_need_ling));
        }
        else
        {
            str_replace(content, "$F", LEX_CAST_STR(m_need_ling/4));
        }
        str_replace(content, "$L", LEX_CAST_STR(m_need_ling));
        std::string reward = "";
        int silver_total = 0, gold_total = 0, ling_total = 0, prestige_total = 0, exp_total = 0;
        std::map<int,int> treasure_list,equipment_list,general_list,zhen_list,skill_list;
        std::vector<boost::shared_ptr<SweepResult> >::iterator it_out = m_sweep_itemlist.begin();
        while (it_out != m_sweep_itemlist.end() && (*it_out).get())
        {
            std::vector<boost::shared_ptr<Item> >::iterator it = (*it_out)->itemlist.begin();
            while (it != (*it_out)->itemlist.end() && (*it).get())
            {
                switch ((*it)->type)
                {
                    case item_type_silver:    //银币
                    {
                        silver_total += (*it)->nums;
                        break;
                    }
                    case item_type_treasure:    //道具
                    {
                        std::map<int,int>::iterator it_m = treasure_list.find((*it)->id);
                        if (it_m != treasure_list.end())
                        {
                            it_m->second += (*it)->nums;
                        }
                        else
                            treasure_list[(*it)->id] = (*it)->nums;
                        break;
                    }
                    case item_type_equipment://装备
                    {
                        std::map<int,int>::iterator it_m = equipment_list.find((*it)->id);
                        if (it_m != equipment_list.end())
                        {
                            ++it_m->second;
                        }
                        else
                            equipment_list[(*it)->id] = 1;
                        break;
                    }
                    case item_type_general://武将
                    {
                        std::map<int,int>::iterator it_m = general_list.find((*it)->id);
                        if (it_m != general_list.end())
                        {
                            ++it_m->second;
                        }
                        else
                            general_list[(*it)->id] = 1;
                        break;
                    }
                    case item_type_zhen:    //阵型
                    {
                        std::map<int,int>::iterator it_m = zhen_list.find((*it)->id);
                        if (it_m != zhen_list.end())
                        {
                            ++it_m->second;
                        }
                        else
                            zhen_list[(*it)->id] = 1;
                        break;
                    }
                    case item_type_skill:    //技能
                    {
                        std::map<int,int>::iterator it_m = skill_list.find((*it)->id);
                        if (it_m != skill_list.end())
                        {
                            ++it_m->second;
                        }
                        else
                            skill_list[(*it)->id] = 1;
                        break;
                    }
                    case item_type_silver_map:
                    {
                        silver_total += (*it)->nums;
                        break;
                    }
                    case item_type_gold:
                    {
                        gold_total += (*it)->nums;
                        break;
                    }
                    case item_type_ling:
                    {
                        ling_total += (*it)->nums;
                        break;
                    }
                    case item_type_prestige:
                    {
                        prestige_total += (*it)->nums;
                        break;
                    }
                    case item_type_exp:
                    {
                        exp_total += (*it)->nums;
                        break;
                    }
                }
                ++it;
            }
            ++it_out;
        }
        std::map<int,int>::iterator it_reward;
        if (!treasure_list.empty())
        {
            it_reward = treasure_list.begin();
            while (it_reward != treasure_list.end())
            {
                boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(it_reward->first);
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
        if (!general_list.empty())
        {
            it_reward = general_list.begin();
            while (it_reward != general_list.end())
            {
                boost::shared_ptr<GeneralTypeData> tr = GeneralDataMgr::getInstance()->GetBaseGeneral(it_reward->first);
                if (tr.get())
                {
                    reward += (tr->m_name + strCounts + LEX_CAST_STR(it_reward->second) + " ");
                }
                ++it_reward;
            }
        }
        if (!zhen_list.empty())
        {
            it_reward = zhen_list.begin();
            while (it_reward != zhen_list.end())
            {
                boost::shared_ptr<BaseZhenData> bz = GeneralDataMgr::getInstance()->GetBaseZhen(it_reward->first);
                if (bz.get())
                {
                    reward += (bz->m_name + strCounts + LEX_CAST_STR(it_reward->second) + " ");
                }
                ++it_reward;
            }
        }
        if (!skill_list.empty())
        {
            it_reward = skill_list.begin();
            while (it_reward != skill_list.end())
            {
                boost::shared_ptr<baseSkill> bs = baseSkillMgr::getInstance()->GetBaseSkill(it_reward->first);
                if (bs.get())
                {
                    reward += (bs->name + strCounts + LEX_CAST_STR(it_reward->second) + " ");
                }
                ++it_reward;
            }
        }
        if (prestige_total != 0)
            reward += (strPrestige + strCounts + LEX_CAST_STR(prestige_total) + " ");
        if (ling_total != 0)
            reward += (strLing + strCounts + LEX_CAST_STR(ling_total) + " ");
        if (gold_total != 0)
            reward += (strGold + strCounts + LEX_CAST_STR(gold_total) + " ");
        if (silver_total != 0)
            reward += (strSilver + strCounts + LEX_CAST_STR(silver_total) + " ");
        if (exp_total != 0)
        {
            reward += (strExp + strCounts + LEX_CAST_STR(exp_total));
        }
        str_replace(content, "$R", reward);
        //cout << "send sweepdoneall!!!" << endl;

        if (account.get())
        {
            json_spirit::Object notify;
            notify.push_back( Pair ("cmd", "sweepDoneAll"));
            notify.push_back( Pair ("msg", content));
            notify.push_back( Pair ("s", 200));
            account->Send(json_spirit::write(notify, json_spirit::raw_utf8));
        }
        else
        {
            sendSystemMail(cdata->m_name, m_cid, strSweepMailTitle, content);
        }
        stop();
    }
    else
    {
        //通知客户端完成一场扫荡
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(cdata->m_name);
        if (account.get())
        {
            json_spirit::Object obj;
            obj.clear();
            obj.push_back( Pair("cmd", "SweepDone") );
            obj.push_back( Pair("s", 200) );
            account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
        }
        //新建下次定时器
        json_spirit::mObject mobj;
        mobj["cmd"] = "sweepDone";
        mobj["cid"] = m_cid;
        mobj["total_time"] = 1;
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(iSweepEliteTime, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return HC_SUCCESS;
}

int charSweep::cancel()
{
    if (m_start_time > 0 && m_end_time > 0)
    {
        m_left_fights = 0;
        m_fast_mod = 0;
        m_start_time = 0;
        m_end_time = 0;
        m_need_ling = m_left_fights;
        if (!_uuid.is_nil())
        {
            splsTimerMgr::getInstance()->delTimer(_uuid);
            _uuid = boost::uuids::nil_uuid();
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int charSweep::stop()
{
    if (m_start_time > 0 && m_end_time > 0)
    {
        m_sweep_task.clear();
        m_left_fights = 0;
        now_sweep_id = 0;
        m_need_ling = 0;
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

int charSweep::addSweepTask(int mapid, int stageid, int strongholdid, int type)
{
    //该玩家在扫荡中
    if (m_start_time != 0)
    {
        return HC_ERROR;
    }
    if (m_sweep_task.size() == 0)
    {
        m_type = type;
        m_mapid = mapid;
    }
    else if (m_type != type)
    {
        return HC_ERROR;
    }
    if (type == 1)//关卡扫荡
    {
        //cout << "cid=" << cid << ",addsweeptask strongholdid=" << strongholdid << ",times=" << times <<endl;
        boost::shared_ptr<StrongholdData> pshd = GeneralDataMgr::getInstance()->GetStrongholdData(strongholdid);
        if (!pshd.get())
            return HC_ERROR;
        boost::shared_ptr<CharStrongholdData> pcsd = GeneralDataMgr::getInstance()->GetCharStrongholdData(m_cid, mapid, stageid, pshd->m_strongholdpos);
        if (!pcsd.get())
            return HC_ERROR;
        //该精英未激活
        if (pcsd->m_state <= 0)
            return HC_ERROR_NO_ATTACK_TIMES;
    }
    else if (type == 2)//战役扫荡
    {
        boost::shared_ptr<CharEliteCombatData> pced = eliteCombatMgr::getInstance()->getCharEliteCombat(m_cid,mapid,strongholdid);
        if (!pced.get())
        {
            return HC_ERROR;
        }
        //未激活
        if (pced->m_state != elite_active)
            return HC_ERROR_NO_ATTACK_TIMES;
    }
    else
    {
        return HC_ERROR;
    }
    m_sweep_task.push_back(strongholdid);
    return HC_SUCCESS;
}

void charSweep::save()
{
    saveDbJob job;
    job.sqls.push_back("delete from char_sweep_result where cid=" + LEX_CAST_STR(m_cid));
    job.sqls.push_back("delete from char_sweep_task where cid=" + LEX_CAST_STR(m_cid));

    if (m_fast_mod == 0)
    {
        std::string tasks = "";
        for (std::list<int>::iterator it = m_sweep_task.begin(); it != m_sweep_task.end(); ++it)
        {
            tasks = tasks + LEX_CAST_STR(*it) + ",";
        }
        job.sqls.push_back("insert into char_sweep_task set cid=" + LEX_CAST_STR(m_cid)
            + ", left_times=" + LEX_CAST_STR(m_left_fights)
            + ", start_time=" + LEX_CAST_STR(m_start_time)
            + ", end_time=" + LEX_CAST_STR(m_end_time)
            + ", total=" + LEX_CAST_STR(m_need_ling)
            + ", now_sweep_id=" + LEX_CAST_STR(now_sweep_id)
            + ", mapid=" + LEX_CAST_STR(m_mapid)
            + ", sweep_task='" + tasks
            + "', type=" + LEX_CAST_STR(m_type));

        if (m_sweep_itemlist.size() > 0)
        {
            std::vector<boost::shared_ptr<SweepResult> >::iterator it2 = m_sweep_itemlist.begin();
            int pos = 1;
            std::string result_sql = "insert into char_sweep_result (cid,pos,stronghold_id,type,id,num) values ";
            std::string insert_sql = "";
            while (it2 != m_sweep_itemlist.end() && (*it2).get())
            {
                std::vector<boost::shared_ptr<Item> >::iterator it_in = (*it2)->itemlist.begin();
                while (it_in != (*it2)->itemlist.end() && (*it_in).get())
                {
                    if (insert_sql != "")
                    {
                        insert_sql += ",";
                    }
                    insert_sql = insert_sql + "(" + LEX_CAST_STR(m_cid)
                                + "," + LEX_CAST_STR(pos)
                                + "," + LEX_CAST_STR((*it2)->stronghold_id)
                                + "," + LEX_CAST_STR((*it_in)->type)
                                 + "," + LEX_CAST_STR((*it_in)->id)
                                 + "," + LEX_CAST_STR((*it_in)->nums) + ")";
                    ++it_in;
                }
                ++pos;
                ++it2;
            }
            job.sqls.push_back(result_sql + insert_sql);
        }
    }
    InsertSaveDb(job);
}

sweepMgr* sweepMgr::m_handle = NULL;

sweepMgr::sweepMgr()
{
}

sweepMgr::~sweepMgr()
{
}

sweepMgr* sweepMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new sweepMgr();
    }
    return m_handle;
}

int sweepMgr::SpeedUp(int cid)
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

int sweepMgr::Done(int cid, int total_time)
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return HC_ERROR;
    boost::shared_ptr<charSweep> pcs = getCharSweepData(cid);
    if (pcs.get())
    {
        int ret = pcs->done(total_time);
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
                obj.push_back( Pair("cmd", "SweepDone") );
                obj.push_back( Pair("s", ret) );
                account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
            }
        }
        cdata->NotifyCharData();
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
    q.get_result("SELECT sweep_task,total,left_times,start_time,end_time,now_sweep_id,type,mapid FROM char_sweep_task WHERE cid=" + LEX_CAST_STR(cid));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {        
        {
            std::string tasks = q.getstr();
            typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
            boost::char_separator<char> sep(",");
            tokenizer tok(tasks, sep);
            tokenizer::iterator it = tok.begin();
            while (it != tok.end())
            {
                int stronghold = atoi((*it).c_str());
                if (stronghold > 0)
                {
                    p->m_sweep_task.push_back(stronghold);
                }
                ++it;
            }
        }
        
        p->m_need_ling = q.getval();
        p->m_left_fights = q.getval();
        if (p->m_left_fights > p->m_need_ling)
        {
            p->m_left_fights = p->m_need_ling;
        }
        p->m_start_time = q.getval();
        p->m_end_time = q.getval();
        if (p->m_end_time < p->m_start_time)
        {
            p->m_end_time = p->m_start_time;
        }
        p->now_sweep_id = q.getval();
        p->m_type = q.getval();
        p->m_mapid = q.getval();
    }    
    q.free_result();
    
    int last_strongholdid = 0;
    boost::shared_ptr<SweepResult> psr;
    q.get_result("SELECT stronghold_id,type,id,num FROM char_sweep_result WHERE cid=" + LEX_CAST_STR(cid) + " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int strongholdid = q.getval();
        if (strongholdid != last_strongholdid)
        {
            if (last_strongholdid != 0)
            {
                p->m_sweep_itemlist.push_back(psr);
            }
            last_strongholdid = strongholdid;
            psr.reset(new SweepResult);
            psr->stronghold_id = strongholdid;
        }
        if (psr.get())
        {
            boost::shared_ptr<Item> pi(new Item);
            pi->type = q.getval();
            pi->id = q.getval();
            pi->nums = q.getval();
            psr->itemlist.push_back(pi);
        }
    }
    q.free_result();
    if (last_strongholdid != 0)
    {
        p->m_sweep_itemlist.push_back(psr);
    }
    m_sweep_task[cid] = p;
    if (!p->m_sweep_task.empty())
        p->_start();
    return p;
}

