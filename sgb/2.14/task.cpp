#include "statistics.h"

#include "task.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "utils_all.h"

#include "data.h"
#include "spls_errcode.h"
#include "eliteCombat.h"
#include "training.h"
#include "spls_race.h"
#include "singleton.h"

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

baseTask::baseTask()
{
    task_type = 0;
    need_task = 0;
    mapid = 0;
    stageid = 0;
    sweep = 0;
}

baseTask::~baseTask()
{
    //cout<<"baseTask::~baseTask("<<id<<")"<<endl;
}

void baseTask::loadRewards()
{
    Query q(GetDb());
    if (task_type == 0)
    {
        //主线任务奖励表
        q.get_result("select itemType,itemId,counts,fac from base_tasks_reward where taskid="+LEX_CAST_STR(id)+" order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.fac = q.getval();
            reward.push_back(item);
        }
        q.free_result();
    }
}

void charTask::Save()
{
    InsertSaveDb("update char_trunk_tasks set need=" + LEX_CAST_STR(need)
        + ",current=" + LEX_CAST_STR(cur)
        + " where cid=" + LEX_CAST_STR(m_charData.m_id)
        + " and tid=" + LEX_CAST_STR(tid)
        );
}

//接受任务
void charTrunkTasks::acceptTask(boost::shared_ptr<const baseTask> t)
{
    if (t.get())
    {
        boost::shared_ptr<charTask> ct(new charTask(m_charData));
        ct->tid = t->id;
        ct->_task = t;
        
        ct->cur = 0;
        ct->done = false;

        switch (t->type)
        {
            case task_attack_stronghold:
                ct->need = t->need[1];
                break;
            case task_rob_stronghold:
                ct->need = t->need[1];
                break;
            case task_get_gem:
                ct->need = t->need[1];
                ct->cur = m_charData.m_bag.getCount((uint16_t) iItem_type_gem, t->need[0]);
                if (ct->cur > ct->need)
                {
                    ct->cur = ct->need;
                }
                break;
            case task_equipment_level:
                //装备等级
                ct->need = t->need[1];
                ct->cur = m_charData.maxEquipLevel(t->need[0]);
                if (ct->cur < 0)
                {
                    ct->cur = 0;
                }
                break;
            case task_weapon_level:    
                {
                    ct->need = t->need[1];
                    baseNewWeapon* pb = newWeaponMgr::getInstance()->getWeapon(ct->_task->need[0]);
                    if (!pb || pb->_type > 5 || pb->_type < 1)
                    {
                        ct->cur = ct->_task->need[1];
                        ct->need = ct->_task->need[1];
                        ct->done = true;
                    }
                    else if (!m_charData.m_new_weapons._weapons[pb->_type-1]._baseWeapon)
                    {
                        ct->cur = 0;
                        ct->need = ct->_task->need[1];
                    }
                    else
                    {
                        ct->cur = m_charData.m_new_weapons._weapons[pb->_type-1]._level;
                        ct->need = ct->_task->need[1];
                    }
                    break;
                }
            case task_equipment_make:
                ct->need = 1;
                if (m_charData.CheckHasEquipt(t->need[0]))
                {
                    ct->cur = ct->need;
                }
                else
                {
                    ct->cur = 0;
                }
                break;
            case task_buy_bag:
                {
                    ct->need = t->need[0];
                    int buyed = m_charData.m_bag.size() - BAG_DEFAULT_SIZE;
                    if (buyed >= t->need[0])
                    {
                        ct->cur = ct->need;
                    }
                    else
                    {
                        ct->cur = 0;
                    }
                }
                break;
            case task_join_corps:    //加入军团
            {
                ct->need = 1;
                if (m_charData.m_corps_member.get())
                {
                    ct->cur = 1;
                }
                else
                {
                    ct->cur = 0;
                }
                break;
            }
            case task_corps_jisi:    //军团祭天
            {
                ct->need = t->need[0];
                ct->cur = m_charData.m_temp_jisi_times;
                break;
            }
            case task_corps_ymsj:    //军团辕门射戟
            {
                ct->need = t->need[0];
                ct->cur = m_charData.queryExtraData(char_data_type_daily, char_data_daily_corps_ymsj);
                break;
            }
            case task_corps_explore: //军团探索
            {
                ct->need = t->need[0];
                ct->cur = m_charData.queryExtraData(char_data_type_daily, char_data_daily_corps_explore);
                break;
            }
            case task_daily_score:   //每日活跃度
            {
                ct->need = t->need[0];
                ct->cur = m_charData.queryExtraData(char_data_type_daily, char_data_daily_task);
                break;
            }
            case task_get_general:        //获得指定武将
            {
                ct->need = 1;
                ct->cur = 0;
                if (m_charData.m_generals.GetGeneralByType(t->need[0]) > 0)
                {
                    ct->cur = 1;
                }
                break;
            }
            case task_gather_gem:
            {
                ct->need = t->need[1];
                ct->cur = 0;
                if (ct->cur > ct->need)
                {
                    ct->cur = ct->need;
                }
                break;
            }
            case task_levy:          //征收 38 次数
            case task_add_friends:   //盟友 39 个数
            case task_train:         //训练 40 次数
            case task_normal_wash:   //普通洗髓 41 次数
            case task_2gold_wash:    //青铜洗髓 42 次数
            case task_horse_train:   //战马培养 44 次数
            case task_arena_win:     //竞技场胜利 次数
            case task_farm_harvest:  //屯田收获 次数
            case task_farm_water:    //屯田浇灌 次数
            case task_farm_yechan:   //屯田野产 次数
            case task_baoshi_exchange:  //宝石兑换
            case task_baoshi_combine:   //宝石合成
            case task_baoshi_convert:   //宝石转换
            case task_arrest_servant:   //抓捕壮丁  次数
            case task_rescue_servant:   //解救壮丁 次数
            case task_reborn:           //重生 次数
            case task_trade_wjbs:       //贸易使用无商不奸
            case task_general_inherit:  //武将传承 次数
            case task_shop_buy_mat:     //商店购买材料 64
            case task_shop_buy_baoshi:  //商店购买宝石 65
            {
                ct->need = t->need[0];
                ct->cur = 0;
                break;
            }
            case task_wash_star:     //洗髓星级 43 
            {
                ct->need = 1;
                ct->cur = 0;
                CharTotalGenerals& char_generals = m_charData.GetGenerals();
                std::list<boost::shared_ptr<CharGeneralData> > char_generals_list;
                char_generals_list.clear();
                std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.m_generals.begin();
                while (it != char_generals.m_generals.end())
                {
                    if (!it->second.get())
                    {
                        ++it;
                        continue;
                    }
                    CharGeneralData* gd = it->second.get();
                    if (gd->m_wash_star.get() && gd->m_wash_star->id >= t->need[0])
                    {
                        ct->cur = 1;
                        break;
                    }
                    ++it;
                }
                break;
            }
            case task_reborn_star:   //重生星级
            {
                ct->need = 1;
                ct->cur = 0;
                CharTotalGenerals& char_generals = m_charData.GetGenerals();
                std::list<boost::shared_ptr<CharGeneralData> > char_generals_list;
                char_generals_list.clear();
                std::map<int, boost::shared_ptr<CharGeneralData> >::iterator it = char_generals.m_generals.begin();
                while (it != char_generals.m_generals.end())
                {
                    if (!it->second.get())
                    {
                        ++it;
                        continue;
                    }
                    CharGeneralData* gd = it->second.get();
                    if (gd->m_chengzhang_star.get() && gd->m_chengzhang_star->id >= t->need[0])
                    {
                        ct->cur = 1;
                        break;
                    }
                    ++it;
                }
                break;
            }                    
            case task_arena_liansheng:      //竞技场连胜
            {
                ct->need = t->need[0];
                ct->cur = 0;
                CharRaceData* rd = RaceMgr::getInstance()->getRaceData(m_charData.m_id).get();
                if (rd && rd->getChar())
                {
                    ct->cur = rd->m_wins;
                }
                break;
            }
            case task_baoshi_combine_level: //宝石合成
            case task_trade_star:           //贸易星级
            {
                ct->need = 1;
                ct->cur = 0;
                break;
            }
            case task_upgrade_soul:  //升级演兵
            case task_maze_score:    //八卦阵评价
            {
                ct->need = 1;
                ct->cur = 0;
                break;
            }
            case task_center_soul_level: //演兵魂眼等级
            {
                ct->need = t->need[1];
                CharTrainings* ctr = Singleton<trainingMgr>::Instance().getChar(m_charData).get();
                if (ctr)
                {
                    ct->cur = ctr->minlevel(t->need[0]);
                }
                else
                {
                    ct->cur = 0;
                }
                break;
            }
            case task_elite_combat:
            default:
                ct->need = 1;
                if (eliteCombatMgr::getInstance()->check_stronghold_can_sweep(m_charData.m_id, t->mapid, t->need[0]))
                {
                    ct->cur = 1;
                }
                break;

        }
        if (ct->cur >= ct->need)
        {
            ct->done = true;
        }
        m_trunk_tasks[t->id] = ct;

        InsertSaveDb("insert into char_trunk_tasks set need=" + LEX_CAST_STR(ct->need)
        + ",current=" + LEX_CAST_STR(ct->cur)
        + ",cid=" + LEX_CAST_STR(m_charData.m_id)
        + ",tid=" + LEX_CAST_STR(ct->tid)
        );
    }
}

//任务列表
int charTrunkTasks::getList(json_spirit::Array& rlist)
{
    bool have_elite_task = false;
    std::map<int, boost::shared_ptr<charTask> >::iterator it = m_trunk_tasks.begin();
    while (it != m_trunk_tasks.end())
    {
        if (it->second.get() && it->second->_task.get())
        {
            charTask& ct = *(it->second.get());
            json_spirit::Object obj;
            obj.push_back( Pair("id", ct._task->id + 1000) );
            obj.push_back( Pair("type", ct._task->type) );
            obj.push_back( Pair("name", ct._task->title) );
            //obj.push_back( Pair("memo", ct._task->memo) );
            obj.push_back( Pair("goal", ct.need) );
            obj.push_back( Pair("current", ct.cur) );
            obj.push_back( Pair("isDone", ct.done ? 2 : 1) );

            if (ct._task->type == task_equipment_level || ct._task->type == task_equipment_make)
            {
                obj.push_back( Pair("equip", ct._task->need[0]) );
            }
            else if (task_weapon_level == ct._task->type)
            {
                //秘法类型
                obj.push_back( Pair("mtype", ct._task->need[0]) );
            }
            if (ct._task->target_stronghold.get())
            {
                obj.push_back( Pair("mapid", ct._task->target_stronghold->m_map_id) );
                obj.push_back( Pair("stageid", ct._task->target_stronghold->m_stage_id) );
                obj.push_back( Pair("pos", ct._task->target_stronghold->m_strongholdpos) );
                if (ct._task->sweep)
                {
                    obj.push_back( Pair("sweep", ct._task->sweep) );
                }
                boost::shared_ptr<CharStrongholdData> cd = GeneralDataMgr::getInstance()->GetCharStrongholdData(m_charData.m_id,ct._task->target_stronghold->m_map_id,ct._task->target_stronghold->m_stage_id,ct._task->target_stronghold->m_strongholdpos);
                if (cd.get())
                {
                    obj.push_back( Pair("state", cd->m_state));
                }
                else
                {
                    obj.push_back( Pair("state", -2));
                }
            }
            else
            {
                if (ct._task->mapid)
                {
                    obj.push_back( Pair("mapid", ct._task->mapid) );
                }
                if (ct._task->stageid)
                {
                    obj.push_back( Pair("stageid", ct._task->stageid) );
                }
            }
            if (ct._task->type != task_elite_combat)
            {
                rlist.push_back(obj);
            }
            //精英战役的任务放在最前面，而且只显示一个
            else if (!have_elite_task)
            {
                have_elite_task = true;
                obj.push_back( Pair("pos", ct._task->need[0]) );
                if (rlist.size() == 0)
                {
                    rlist.push_back(obj);
                }
                else
                {
                    rlist.insert(rlist.begin() + 1, obj);
                }
            }
        }
        ++it;
    }
    return HC_SUCCESS;
}

//更新任务
int charTrunkTasks::updateTask(int type, int n1, int n2)
{
    bool bnotify = false;
    std::map<int, boost::shared_ptr<charTask> >::iterator it = m_trunk_tasks.begin();
    while (it != m_trunk_tasks.end())
    {
        if (it->second.get() && it->second->_task.get() && it->second->_task->type == type)
        {
            //只有收集物品的任务，在完成后，可能变为未完成
            if (it->second->done)
            {
                charTask& ct = *(it->second.get());
                if (type == task_get_gem && n1 == ct._task->need[0])
                {
                    bnotify = true;
                    ct.cur = n2;
                    if (ct.cur >= ct.need)
                    {
                        ct.cur = ct.need;
                        ct.done = true;
                    }
                    else
                    {
                        ct.done = false;
                    }
                    ct.Save();
                }
            }
            else
            {
                charTask& ct = *(it->second.get());
                switch (type)
                {
                    case task_attack_stronghold:
                    case task_rob_stronghold:
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur += n2;
                            if (ct.cur >= ct.need)
                            {
                                ct.cur = ct.need;
                                ct.done = true;
                            }
                            ct.Save();
                        }
                        break;
                    case task_get_gem:
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur = n2;
                            if (ct.cur >= ct.need)
                            {
                                ct.cur = ct.need;
                                ct.done = true;
                            }
                            ct.Save();
                        }
                        break;
                    case task_elite_combat:
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur += n2;
                            if (ct.cur >= ct.need)
                            {
                                ct.cur = ct.need;
                                ct.done = true;
                            }
                            ct.Save();
                        }
                        break;
                    case task_equipment_level:    //装备等级
                        if (n1 == ct._task->need[0] && n2 > ct.cur)
                        {
                            bnotify = true;
                            ct.cur = n2;
                            if (n2 >= ct._task->need[1])
                            {
                                ct.cur = ct._task->need[1];
                                if (ct.cur == 0)
                                {
                                    ct.cur = 1;
                                }
                                ct.done = true;
                            }
                            ct.Save();
                        }
                        break;
                    case task_weapon_level:    //兵器等级
                    {
                        if (n1 == ct._task->need[0])
                        {
                            ct.cur = n2;
                            bnotify = true;
                            if (n2 >= ct._task->need[1])
                            {
                                ct.cur = ct._task->need[1];
                                if (ct.cur == 0)
                                {
                                    ct.cur = 1;
                                }
                                ct.done = true;
                            }
                            ct.Save();
                        }
                        else if (n1 > ct._task->need[0] && (n1 - ct._task->need[0])%5 == 0)
                        {
                            ct.cur = ct._task->need[1];
                            ct.done = true;
                            bnotify = true;
                        }
                    }
                    break;
                    case task_equipment_make:    //装备制造
                    {
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur = ct.need;
                            ct.done = true;
                            ct.Save();
                        }
                    }
                    break;
                    case task_buy_bag:
                    {
                        if (n1 >= ct._task->need[0])
                        {
                            ct.cur = ct.need;
                            ct.done = true;
                            bnotify = true;
                        }
                        break;
                    }
                    case task_gather_gem:
                    {
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur += n2;
                            if (ct.cur >= ct.need)
                            {
                                ct.cur = ct.need;
                                ct.done = true;
                            }
                            ct.Save();
                        }
                        break;
                    }
                    case task_join_corps:
                    {
                        bnotify = true;
                        ct.done = true;
                        ct.cur = ct.need;
                        ct.Save();
                        break;
                    }
                    case task_corps_jisi:
                    case task_corps_ymsj:
                    case task_corps_explore:
                    {
                        bnotify = true;
                        ct.cur += n1;
                        if (ct.cur >= ct.need)
                        {
                            ct.cur = ct.need;
                            ct.done = true;
                        }
                        ct.Save();
                        break;
                    }
                    case task_daily_score:
                    {
                        bnotify = true;
                        ct.cur = n1;
                        if (ct.cur >= ct.need)
                        {
                            ct.cur = ct.need;
                            ct.done = true;
                        }
                        ct.Save();
                        break;
                    }
                    case task_get_general:
                    {
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur = ct.need;
                            ct.done = true;
                            ct.Save();
                        }
                        break;
                    }
                    
                    case task_levy:          //征收 38 次数
                    case task_add_friends:   //盟友 39 个数
                    case task_train:         //训练 40 次数
                    case task_normal_wash:   //普通洗髓 41 次数
                    case task_2gold_wash:    //青铜洗髓 42 次数
                    case task_horse_train:   //战马培养 44 次数
                    case task_arena_win:     //竞技场胜利 次数
                    case task_farm_harvest:  //屯田收获 次数
                    case task_farm_water:    //屯田浇灌 次数
                    case task_farm_yechan:   //屯田野产 次数
                    case task_baoshi_exchange:  //宝石兑换
                    case task_baoshi_combine:   //宝石合成
                    case task_baoshi_convert:   //宝石转换
                    case task_arrest_servant:   //抓捕壮丁  次数
                    case task_rescue_servant:   //解救壮丁 次数
                    case task_reborn:           //重生 次数
                    case task_trade_wjbs:       //贸易使用无商不奸
                    case task_general_inherit:  //武将传承 次数
                    case task_shop_buy_mat:     //商店购买材料 64
                    case task_shop_buy_baoshi:  //商店购买宝石 65
                    {
                        bnotify = true;
                        ct.cur += n1;
                        if (ct.cur >= ct.need)
                        {
                            ct.cur = ct.need;
                            ct.done = true;
                        }
                        ct.Save();
                        break;
                    }                    
                    case task_wash_star:     //洗髓星级 43 
                    case task_reborn_star:   //重生星级
                    {
                        if (n1 >= ct._task->need[0])
                        {
                            bnotify = true;                            
                            ct.cur = ct.need;
                            ct.done = true;
                            ct.Save();
                        }
                        break;
                    }                    
                    case task_arena_liansheng:      //竞技场连胜
                    {
                        bnotify = true;
                        ct.cur = n1;
                        if (ct.cur >= ct.need)
                        {
                            ct.cur = ct.need;
                            ct.done = true;
                        }
                        ct.Save();
                        break;
                    }
                    case task_baoshi_combine_level: //宝石合成
                    case task_trade_star:           //贸易星级
                    case task_maze_score:   //八卦阵评价
                    {
                        if (n1 == ct._task->need[0])
                        {
                            bnotify = true;
                            ct.cur = ct.need;
                            ct.done = true;
                            ct.Save();
                        }
                        break;
                    }
                    case task_upgrade_soul:  //升级演兵
                    {
                        if (n1 == ct._task->need[0] && n2 == ct._task->need[1])
                        {
                            bnotify = true;
                            ct.cur = ct.need;
                            ct.done = true;
                            ct.Save();
                        }
                        break;
                    }
                    case task_center_soul_level: //演兵魂眼等级
                    {
                        if (n1 == ct._task->need[0])
                        {
                            if (n2 >= ct._task->need[1])
                            {
                                ct.cur = ct.need;
                                ct.done = true;
                            }
                            else
                            {
                                ct.cur = n2;
                            }
                            bnotify = true;
                            ct.Save();
                        }
                        break;
                    }
                }
            }
            //通知任务变化
            #if 0
            if (bnotify)
            {
                //通知玩家任务完成或者有变化
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_charData.m_name);
                if (account.get())
                {
                    json_spirit::Object obj;
                    obj.push_back( Pair("cmd", "updateTask") );
                    obj.push_back( Pair("s", 200) );
                    obj.push_back( Pair("id", ct._task->id + 1000) );
                    obj.push_back( Pair("current", ct.cur) );
                    obj.push_back( Pair("isDone", ct.done ? 2 : 1) );
                    account->Send(json_spirit::write(obj, json_spirit::raw_utf8));
                }
            }
            #endif
        }
        ++it;
    }
    if (bnotify)
    {
        //通知玩家任务完成或者有变化
        boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_charData.m_name);
        if (account.get())
        {
            json_spirit::Object robj;
            robj.push_back( Pair("cmd", "getCurTask") );
            robj.push_back( Pair("s", 200) );
            taskMgr::getInstance()->getTaskInfo(m_charData, 0, robj);
            account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
        }
    }
    return HC_SUCCESS;
}

//任务完成
int charTrunkTasks::taskDone(int id, json_spirit::Object& robj)
{
    std::map<int, boost::shared_ptr<charTask> >::iterator it = m_trunk_tasks.find(id);
    if (it == m_trunk_tasks.end() || !it->second.get())
    {
        return HC_ERROR;
    }
    charTask& t = *(it->second.get());
      if (!t.done || !t._task.get())
    {
        return HC_ERROR;
    }
    //完成任务，扣任务物品
    if (task_get_gem == t._task->type && t._task->need[0] > 0 && t._task->need[1] > 0)
    {
        int err_code = 0;
        m_charData.m_bag.addGem(t._task->need[0], -t._task->need[1], err_code, false);
    }
    if (t._task->type != task_empty)
    {
        //给奖励
        std::string award = "";
        for (std::list<Item>::const_iterator it = t._task->reward.begin(); it != t._task->reward.end(); ++it)
        {
            switch (it->type)
            {
                case item_type_gold:
                    m_charData.addGold(it->nums);
                    //金币获得统计
                    add_statistics_of_gold_get(m_charData.m_id, m_charData.m_ip_address,it->nums,gold_get_task, m_charData.m_union_id, m_charData.m_server_id);
#ifdef QQ_PLAT
                    gold_get_tencent(&m_charData,it->nums);
#endif
                    break;
                case item_type_silver:
                    m_charData.addSilver(it->nums);
                    //银币获得统计
                    add_statistics_of_silver_get(m_charData.m_id,m_charData.m_ip_address,it->nums,silver_get_task, m_charData.m_union_id, m_charData.m_server_id);
                    break;
                case item_type_ling:
                    m_charData.addLing(it->nums);
                    //军令统计
                    add_statistics_of_ling_cost(m_charData.m_id,m_charData.m_ip_address,it->nums,ling_task,1, m_charData.m_union_id, m_charData.m_server_id);
                    break;
                case item_type_treasure:
                    //宝物获得统计
                    add_statistics_of_treasure_cost(m_charData.m_id,m_charData.m_ip_address,it->id,it->nums,treasure_task,1, m_charData.m_union_id, m_charData.m_server_id);
                    m_charData.addTreasure(it->id, it->nums);
                    break;
                default:
                    ERR();    //其它暂时没有
                    break;
            }
            if (award != "")
            {
                award += "," + it->toString();
            }
            else
            {
                award = it->toString();
            }
        }
        robj.push_back( Pair("msg", award) );
    }
    #if 0
    //开启支线任务
    if (t._task->m_trunk_tasks.size() > 0)
    {
        std::list<boost::shared_ptr<const baseTask> >::const_iterator it_i = t._task->m_trunk_tasks.begin();
        while (it_i != t._task->m_trunk_tasks.end())
        {
            if (it_i->get())
            {
                acceptTask(*it_i);
            }
            ++it_i;
        }
    }
    #endif
    //任务完成了，删除这个任务
    InsertSaveDb("delete from char_trunk_tasks where cid=" + LEX_CAST_STR(m_charData.m_id) + " and tid=" + LEX_CAST_STR(t.tid));
    m_trunk_tasks.erase(it);

    //支线任务完成，开启新支线任务
    taskMgr::getInstance()->acceptTrunkTask2(m_charData, id);
    return HC_SUCCESS;
}

//是否有未完成的支线
bool charTrunkTasks::getFinishState()
{
    bool all_finish = true;
    std::map<int, boost::shared_ptr<charTask> >::iterator it = m_trunk_tasks.begin();
    while (it != m_trunk_tasks.end())
    {
        if (it->second.get() && it->second->_task.get() && it->second.get()->done == 0)
        {
            all_finish = false;
            break;
        }
        ++it;
    }
    return all_finish;
}

taskMgr* taskMgr::m_handle = NULL;

taskMgr* taskMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new taskMgr();
    }
    return m_handle;
}

taskMgr::taskMgr()
{
    Query q(GetDb());

    //基础任务表
    q.get_result("select id,title,memo,type,need1,need2,stronghold_id,done_level from base_tasks where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        baseTask* pt = new baseTask();
        pt->id = id;
        pt->title = q.getstr();
        pt->memo = q.getstr();
        pt->type = q.getval();
        pt->need[0] = q.getval();
        pt->need[1] = q.getval();

        //是否有目标关卡
        int stronghold_id = q.getval();
        pt->done_level = q.getval();

        //任务奖励
        pt->loadRewards();
        /*Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();
        pt->reward.push_back(item);

        经验奖励
        item.type = item_type_exp;
        item.id = 0;
        item.nums = q.getval();
        pt->reward.push_back(item);*/

        if (task_attack_stronghold == pt->type)
        {
            stronghold_id = pt->need[0];
        }
        if (stronghold_id > 0)
        {
            pt->target_stronghold = GeneralDataMgr::getInstance()->GetStrongholdData(stronghold_id);
        }

        if (task_enter_map == pt->type)
        {
            pt->mapid = pt->need[0];
        }
        else if (task_enter_stage == pt->type)
        {
            pt->mapid = pt->need[0];
            pt->stageid = pt->need[1];
        }

        pt->detail_obj.push_back( Pair("id", id) );
        pt->detail_obj.push_back( Pair("name", pt->title) );
        pt->detail_obj.push_back( Pair("memo", pt->memo) );
        int goal = 1;
        switch (pt->type)
        {
            case task_char_level:        //主将达到指定等级
                goal = pt->need[0];
                break;
            case task_general_level:    //5 武将升级到一定等级
            case task_group_general_level://n个武将升级到一定等级
                goal = pt->need[1];
                break;
            case task_skill_level:        //技能升级到一定等级
                goal = pt->need[1];
                break;
            case task_zhen_level:        //阵型升级到指定等级
                goal = pt->need[1];
                break;
        }
        pt->detail_obj.push_back( Pair("goal", goal) );
        pt->detail_obj.push_back( Pair("current", goal) );
        pt->detail_obj.push_back( Pair("isDone", 3) );

        std::string award = "";
        for (std::list<Item>::iterator it = pt->reward.begin(); it != pt->reward.end(); ++it)
        {
            if (award != "")
            {
                award += "," + it->toString();
            }
            else
            {
                award = it->toString();
            }
        }
        pt->detail_obj.push_back( Pair("award", award) );

        pt->simple_obj.push_back( Pair("id", id) );
        pt->simple_obj.push_back( Pair("name", pt->title) );
        pt->simple_obj.push_back( Pair("isDone", 3) );

        assert(id == (int)(m_total_tasks.size() + 1));

        boost::shared_ptr<const baseTask> bt(pt);
        m_total_tasks.push_back(bt);
    }
    q.free_result();

    baseTask* pt = new baseTask();
    pt->id = m_total_tasks.size() + 1;
    pt->title = "";
    pt->memo = "";
    pt->type = task_empty;
    pt->need[0] = 0;
    pt->need[1] = 0;
    //pt->reward.type = 0;
    //pt->reward.id = 0;
    //pt->reward.nums = 0;
    boost::shared_ptr<const baseTask> bt(pt);
    m_total_tasks.push_back(bt);

    //支线任务表
    q.get_result("select id,title,memo,type,need1,need2,itemType,itemId,counts,stronghold,pre_task,extra from base_trunk_tasks where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        baseTask* pt = new baseTask();
        pt->task_type = 1;
        pt->id = id;
        pt->title = q.getstr();
        pt->memo = q.getstr();
        pt->type = q.getval();
        pt->need[0] = q.getval();
        pt->need[1] = q.getval();

        //任务奖励
        Item item;
        item.type = q.getval();
        item.id = q.getval();
        item.nums = q.getval();

        #if 0
        int pre_task = q.getval();

        boost::shared_ptr<const baseTask> ptask = getTask(pre_task);
        if (pre_task > 1000)
        {
            ptask = getTrunkTask(pre_task - 1000);
        }
        if (!ptask.get())
        {
            ERR();
            cout<<"trunk tid:"<<id<<",pre "<<pre_task<<endl;
            continue;
        }
        #else
        int get_strongholdid = q.getval();
        int pre_task = q.getval();
        #endif

        int extra = q.getval();
        
        pt->reward.push_back(item);

        //是否有目标关卡
        int stronghold_id = 0;

        if (task_rob_stronghold == pt->type || task_attack_stronghold == pt->type)
        {
            stronghold_id = pt->need[0];
        }
        //收集任务
        if (task_get_gem == pt->type || task_gather_gem == pt->type)
        {
            baseTreasure* bt = GeneralDataMgr::getInstance()->GetBaseTreasure(pt->need[0]).get();
            if (bt && bt->m_place.get())
            {
                lootPlaceInfo* lt = bt->m_place.get();
                pt->target_stronghold = GeneralDataMgr::getInstance()->GetStrongholdData(lt->mapId, lt->stageId, lt->pos);
                if (extra == 0)
                {
                    pt->sweep = 1;
                }
            }
        }
        if (stronghold_id > 0)
        {
            pt->target_stronghold = GeneralDataMgr::getInstance()->GetStrongholdData(stronghold_id);
        }
        if (task_elite_combat == pt->type)
        {
            boost::shared_ptr<eliteCombat> elite = eliteCombatMgr::getInstance()->getEliteCombatById(pt->need[0]);
            if (elite.get())
            {
                pt->mapid = elite->_mapid;
                get_strongholdid = elite->_open_stronghold;
            }
            if (pt->need[1] < 1)
            {
                pt->need[1] = 1;
            }
        }
        pt->detail_obj.push_back( Pair("id", id) );
        pt->detail_obj.push_back( Pair("name", pt->title) );
        pt->detail_obj.push_back( Pair("memo", pt->memo) );
        int goal = 1;
        switch (pt->type)
        {
            case task_attack_stronghold:
            case task_rob_stronghold:
            case task_get_gem:
            case task_gather_gem:
            case task_equipment_level:
                goal = pt->need[1];
                break;
        }
        pt->detail_obj.push_back( Pair("goal", goal) );
        pt->detail_obj.push_back( Pair("current", goal) );
        pt->detail_obj.push_back( Pair("isDone", 3) );

        std::string award = "";
        for (std::list<Item>::iterator it = pt->reward.begin(); it != pt->reward.end(); ++it)
        {
            if (award != "")
            {
                award += "," + it->toString();
            }
            else
            {
                award = it->toString();
            }
        }
        pt->detail_obj.push_back( Pair("award", award) );

        pt->simple_obj.push_back( Pair("id", id) );
        pt->simple_obj.push_back( Pair("name", pt->title) );
        pt->simple_obj.push_back( Pair("isDone", 3) );

        //收集物品任务
        if (pt->type == task_get_gem)
        {
            boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(pt->need[0]);
            if (bt.get())
            {
                bt->b_used_for_task = true;
            }
        }

        boost::shared_ptr<const baseTask> bt(pt);

        //baseTask* ppt = (baseTask*)(ptask.get());
        //ppt->m_trunk_tasks.push_back(bt);
        if (get_strongholdid)
        {
            std::vector<boost::shared_ptr<const baseTask> >& tasks = m_get_trunk_tasks[get_strongholdid];
            //支线任务领取条件关联到关卡
            tasks.push_back(bt);
        }
        else
        {
            std::vector<boost::shared_ptr<const baseTask> >& tasks = m_get_trunk_tasks2[pre_task];
            //支线任务领取条件关联到关卡
            tasks.push_back(bt);
        }

        assert(id == (int)(m_trunk_tasks.size() + 1));
        m_trunk_tasks.push_back(bt);
    }
    q.free_result();

    cout<<"taskMgr::taskMgr()"<<endl;

}

boost::shared_ptr<const baseTask> taskMgr::getTask(int tid) //根据任务id获得任务
{
    if (tid >= 1 && tid <= (int)m_total_tasks.size())
    {
        return m_total_tasks[tid-1];
    }
    else
    {
        boost::shared_ptr<const baseTask> tt;
        return tt;
    }
}

boost::shared_ptr<const baseTask> taskMgr::getTrunkTask(int tid) //根据任务id获得任务
{
    if (tid >= 1 && tid <= (int)m_trunk_tasks.size())
    {
        return m_trunk_tasks[tid-1];
    }
    else
    {
        boost::shared_ptr<const baseTask> tt;
        return tt;
    }
}

int taskMgr::newChar(boost::shared_ptr<CharData> cdata)
{
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    cdata->m_task._task = getTask(1);
    cdata->m_task.tid = 1;
    cdata->m_task.done = false;
    cdata->m_task.cur = 0;
    cdata->m_task.need = 1;
    return HC_SUCCESS;
}

int taskMgr::queryCurTask(boost::shared_ptr<CharData> cdata)    //获得角色当前任务
{
    //cout<<"queryCurTask()"<<endl;
    if (!cdata.get())
    {
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<const baseTask> >::iterator it = m_char_tasks.find(cdata->m_id);    //角色任务记录
    if (it != m_char_tasks.end())
    {
        cdata->m_task._task.reset();
        cdata->m_task._task = it->second;
        if (cdata->m_task._task.get())
        {
            cdata->m_task.tid = it->second->id;
        }
        else
        {
            ERR();
        }
    }
    else
    {
        int tid = 1;
        cdata->m_task.done = false;
        Query q(GetDb());
        q.get_result("select tid,state from char_tasks where cid=" + LEX_CAST_STR(cdata->m_id));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            tid = q.getval();
            cdata->m_task.done = (q.getval() == 1);
        }
        q.free_result();
        cdata->m_task.tid = tid;
        cdata->m_task._task = getTask(tid);
        if (cdata->m_task._task == NULL)
        {
            cdata->m_task._task = getTask(1);
            cdata->m_task.tid = 1;
            cdata->m_task.done = false;
        }
        if (cdata->m_task.done)
        {
            cdata->init_task_done();
        }

        //从数据库读支线任务记录
        q.get_result("select tid,current from char_trunk_tasks where cid=" + LEX_CAST_STR(cdata->m_id) + " order by tid");
        while (q.fetch_row())
        {
            int tid = q.getval();
            int current = q.getval();

            boost::shared_ptr<const baseTask> bt = getTrunkTask(tid);
            if (!bt.get())
            {
                continue;
            }
            boost::shared_ptr<charTask> ct(new charTask(*(cdata.get())));
            ct->tid = tid;
            ct->_task = bt;
            switch (bt->type)
            {
                case task_attack_stronghold:
                case task_rob_stronghold:
                    ct->need = bt->need[1];
                    break;
                case task_get_gem:
                case task_gather_gem:
                    ct->need = bt->need[1];
                    current = cdata->m_bag.getCount((uint16_t) iItem_type_gem, bt->need[0]);                    
                    break;
                case task_equipment_level:
                    ct->need = bt->need[1];
                    //装备等级
                    current = cdata->maxEquipLevel(bt->need[0]);
                    if (current < 0)
                    {
                        current = 0;
                    }
                    break;
                case task_weapon_level:
                {
                    baseNewWeapon* pb = newWeaponMgr::getInstance()->getWeapon(ct->_task->need[0]);
                    if (!pb || pb->_type > 5 || pb->_type < 1)
                    {
                        ct->cur = ct->_task->need[1];
                        ct->need = ct->_task->need[1];
                        current = ct->cur;
                        ct->done = true;
                    }
                    else if (!cdata->m_new_weapons._weapons[pb->_type-1]._baseWeapon)
                    {
                        ct->cur = 0;
                        current = ct->cur;
                        ct->need = ct->_task->need[1];
                    }
                    else
                    {
                        current = cdata->m_new_weapons._weapons[pb->_type-1]._level;
                        ct->need = ct->_task->need[1];
                    }
                    break;
                }
                case task_daily_score:
                {
                    ct->need = ct->_task->need[0];
                    if (current >= ct->need)
                    {
                        
                    }
                    else
                    {
                        current = cdata->queryExtraData(char_data_type_daily, char_data_daily_task);
                    }
                    break;
                }
                case task_corps_jisi:
                case task_corps_ymsj:
                case task_corps_explore:
                {
                    ct->need = ct->_task->need[0];
                    break;
                }
                case task_get_general:        //获得指定武将
                {
                    ct->need = 1;
                    if (cdata->m_generals.GetGeneralByType(ct->_task->need[0]) > 0)
                    {
                        current = 1;
                    }
                    else
                    {
                        current = 0;
                    }
                    break;
                }
                case task_levy:          //征收 38 次数
                case task_add_friends:   //盟友 39 个数
                case task_train:         //训练 40 次数
                case task_normal_wash:   //普通洗髓 41 次数
                case task_2gold_wash:    //青铜洗髓 42 次数
                case task_horse_train:   //战马培养 44 次数
                case task_arena_win:     //竞技场胜利 次数
                case task_farm_harvest:  //屯田收获 次数
                case task_farm_water:    //屯田浇灌 次数
                case task_farm_yechan:   //屯田野产 次数
                case task_baoshi_exchange:  //宝石兑换
                case task_baoshi_combine:   //宝石合成
                case task_baoshi_convert:   //宝石转换
                case task_arrest_servant:   //抓捕壮丁  次数
                case task_rescue_servant:   //解救壮丁 次数
                case task_reborn:           //重生 次数
                case task_trade_wjbs:       //贸易使用无商不奸
                case task_general_inherit:  //武将传承 次数
                case task_shop_buy_mat:     //商店购买材料 64
                case task_shop_buy_baoshi:  //商店购买宝石 65
                {
                    ct->need = bt->need[0];
                    break;
                }                    
                case task_wash_star:     //洗髓星级 43 
                case task_reborn_star:   //重生星级
                {
                    ct->need = 1;
                    break;
                }                    
                case task_arena_liansheng:      //竞技场连胜
                {
                    ct->need = bt->need[0];
                    break;
                }
                case task_baoshi_combine_level: //宝石合成
                case task_trade_star:           //贸易星级
                {
                    ct->need = 1;
                    break;
                }
                case task_upgrade_soul:  //升级演兵
                {
                    ct->need = 1;
                    break;
                }
                case task_center_soul_level: //演兵魂眼等级
                {
                    ct->need = bt->need[1];                    
                    break;
                }
                case task_elite_combat:
                default:
                    ct->need = 1;
                    break;
            }
            ct->cur = current;
            if (ct->cur >= ct->need)
            {
                ct->cur = ct->need;
                ct->done = true;
            }
            else
            {
                ct->done = false;
            }
            cdata->m_trunk_tasks.m_trunk_tasks[bt->id] = ct;
        }
        q.free_result();
    }
    cdata->checkTask();
    return HC_SUCCESS;
}

int taskMgr::taskDone(boost::shared_ptr<CharData> cdata, int id, json_spirit::Object& robj)        //角色完成任务，领取奖励
{
    if (id > 1000)
    {
        return cdata->m_trunk_tasks.taskDone(id - 1000, robj);
    }
    if (!cdata->m_task.done)
    {
        return HC_ERROR;
    }
    if (!cdata->m_task._task.get())
    {
        return HC_ERROR;
    }
    if (cdata->m_task._task->type == task_empty)
    {
        return HC_ERROR;
    }
    //给奖励
    std::string award = "";
    for (std::list<Item>::const_iterator it = cdata->m_task._task->reward.begin(); it != cdata->m_task._task->reward.end(); ++it)
    {
        switch (it->type)
        {
            case item_type_gold:
                cdata->addGold(it->nums);
                //金币获得统计
                add_statistics_of_gold_get(cdata->m_id,cdata->m_ip_address,it->nums,gold_get_task, cdata->m_union_id, cdata->m_server_id);
                break;
            case item_type_silver:
                cdata->addSilver(it->nums);
                //银币获得统计
                add_statistics_of_silver_get(cdata->m_id,cdata->m_ip_address,it->nums,silver_get_task, cdata->m_union_id, cdata->m_server_id);
                break;
            case item_type_ling:
                cdata->addLing(it->nums);
                //军令统计
                add_statistics_of_ling_cost(cdata->m_id,cdata->m_ip_address,it->nums,ling_task,1, cdata->m_union_id, cdata->m_server_id);
                break;
            case item_type_treasure:
                //宝物获得统计
                add_statistics_of_treasure_cost(cdata->m_id,cdata->m_ip_address,it->id,it->nums,treasure_task,1, cdata->m_union_id, cdata->m_server_id);
                cdata->addTreasure(it->id, it->nums);
                break;
            //case item_type_exp:
            //    cdata->addExp(it->nums);
            //    //经验获得统计?
            //    break;
            default:
                ERR();    //其它暂时没有
                break;
        }
        if (award != "")
        {
            award += "," + it->toString();
        }
        else
        {
            award = it->toString();
        }
        robj.push_back( Pair("msg", award) );
    }
    ++cdata->m_task.tid;
    #if 0
    //开启支线任务
    if (cdata->m_task._task->m_trunk_tasks.size() > 0)
    {
        std::list<boost::shared_ptr<const baseTask> >::const_iterator it = cdata->m_task._task->m_trunk_tasks.begin();
        while (it != cdata->m_task._task->m_trunk_tasks.end())
        {
            if (it->get())
            {
                cdata->m_trunk_tasks.acceptTask(*it);
            }
            ++it;
        }
    }
    #endif
    cdata->m_task._task = getTask(cdata->m_task.tid);

    //新手引导触发
    cdata->checkGuide(guide_type_gettask, cdata->m_task.tid, 0);

    cdata->m_task.done = false;
    cdata->checkTask();
    InsertSaveDb("replace into char_tasks (cid,tid,state) values ("
            + LEX_CAST_STR(cdata->m_id) + ","
            + LEX_CAST_STR(cdata->m_task.tid) + ","
            + (cdata->m_task.done ? "1" : "0") + ")");

    return HC_SUCCESS;
}

//任务列表
int taskMgr::getTaskList(CharData& cData, int page, int pageNums, json_spirit::Object& robj)
{
    if (pageNums < 3)
    {
        pageNums = 3;
    }
    if (!cData.m_task._task.get())
    {
        return HC_ERROR;
    }
    if (cData.m_task.tid > (int)m_total_tasks.size())
    {
        ERR();
        return HC_ERROR;
    }

    int max_page = (cData.m_task.tid + pageNums - 1) / pageNums;
    if (page > max_page || page <= 0)
    {
        page = max_page;
    }
    int from = (page - 1) * pageNums + 1;
    int to = from + pageNums - 1;

    bool showCur = false;
    if (to >= cData.m_task.tid)
    {
        to = cData.m_task.tid-1;
        showCur = true;
    }

    json_spirit::Array tList;
    for (int i = from; i <= to; ++i)
    {
        tList.push_back(m_total_tasks[i - 1]->simple_obj);
    }
    if (showCur && cData.m_task._task->type != task_empty)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("id", cData.m_task._task->id) );
        obj.push_back( Pair("name", cData.m_task._task->title) );    
        obj.push_back( Pair("isDone", cData.m_task.done ? 2 : 1) );

        tList.push_back(obj);
    }

    robj.push_back( Pair("list", tList) );

    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", max_page) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", pageNums) );
    robj.push_back( Pair("page", pageobj) );

    return HC_SUCCESS;
}

//任务详情
int taskMgr::getTaskInfo(CharData& cData, int tid, json_spirit::Object& robj)
{
    if (!cData.m_task._task.get())
    {
        return HC_ERROR;
    }
    if (tid > cData.m_task.tid || tid <= 0)
    {
        tid = cData.m_task.tid;
    }
    if (cData.m_task.tid > (int)m_total_tasks.size())
    {
        ERR();
        return HC_ERROR;
    }
    if (tid == cData.m_task.tid)
    {
        json_spirit::Array rlist;
        if (cData.m_task._task->type != task_empty)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", cData.m_task._task->id) );
            obj.push_back( Pair("primary", 1) );
            obj.push_back( Pair("type", cData.m_task._task->type) );
            obj.push_back( Pair("name", cData.m_task._task->title) );
            //obj.push_back( Pair("memo", cData.m_task._task->memo) );
            obj.push_back( Pair("goal", cData.m_task.need) );
            obj.push_back( Pair("current", cData.m_task.cur) );
            obj.push_back( Pair("isDone", cData.m_task.done ? 2 : 1) );

            std::string award = "";
            for (std::list<Item>::const_iterator it = cData.m_task._task->reward.begin(); it != cData.m_task._task->reward.end(); ++it)
            {
                if (award != "")
                {
                    award += "," + it->toString();
                }
                else
                {
                    award = it->toString();
                }
            }
            obj.push_back( Pair("award", award) );

            //任务对应的关卡位置
            if (cData.m_task.cur < cData.m_task.need)
            {
                if (cData.m_task._task->target_stronghold.get())
                {
                    obj.push_back( Pair("mapid", cData.m_task._task->target_stronghold->m_map_id) );
                    obj.push_back( Pair("stageid", cData.m_task._task->target_stronghold->m_stage_id) );
                    obj.push_back( Pair("pos", cData.m_task._task->target_stronghold->m_strongholdpos) );

                    boost::shared_ptr<CharStrongholdData> cd = GeneralDataMgr::getInstance()->GetCharStrongholdData(cData.m_id,cData.m_task._task->target_stronghold->m_map_id,cData.m_task._task->target_stronghold->m_stage_id,cData.m_task._task->target_stronghold->m_strongholdpos);
                    if (cd.get())
                    {
                        obj.push_back( Pair("state", cd->m_state));
                    }
                    else
                    {
                        obj.push_back( Pair("state", -2));
                    }
                }
                else
                {
                    if (cData.m_task._task->mapid)
                    {
                        obj.push_back( Pair("mapid", cData.m_task._task->mapid) );
                    }
                    if (cData.m_task._task->stageid)
                    {
                        obj.push_back( Pair("stageid", cData.m_task._task->stageid) );
                    }
                }
            }

            rlist.push_back(obj);
        }

        //支线任务信息
        if (cData.m_trunk_tasks.m_trunk_tasks.size() > 0)
        {            
            cData.m_trunk_tasks.getList(rlist);            
        }
        robj.push_back( Pair("list", rlist) );
    }
    else if (m_total_tasks[tid - 1].get())
    {
        robj.push_back( Pair("info", m_total_tasks[tid - 1]->detail_obj) );
    }
    return HC_SUCCESS;
}

//删除角色信息
int taskMgr::deleteChar(int cid)
{
    InsertSaveDb("delete from char_tasks where cid=" + LEX_CAST_STR(cid));
    m_char_tasks.erase(cid);
    return HC_SUCCESS;
}

int taskMgr::acceptTrunkTask(CharData& cData, int strongholdid)
{
    std::map<int, std::vector<boost::shared_ptr<const baseTask> > >::const_iterator it = m_get_trunk_tasks.find(strongholdid);
    if (it != m_get_trunk_tasks.end())
    {
        const std::vector<boost::shared_ptr<const baseTask> >& tasks = it->second;
        if (tasks.size())
        {
            for (std::vector<boost::shared_ptr<const baseTask> >::const_iterator it2 = tasks.begin(); it2 != tasks.end(); ++it2)
            {
                cData.m_trunk_tasks.acceptTask(*it2);
            }
            return HC_SUCCESS;
        }
        ++it;
    }
    return HC_ERROR;
}

int taskMgr::acceptTrunkTask2(CharData& cData, int tid)
{
    std::map<int, std::vector<boost::shared_ptr<const baseTask> > >::const_iterator it = m_get_trunk_tasks2.find(tid);
    if (it != m_get_trunk_tasks2.end())
    {
        const std::vector<boost::shared_ptr<const baseTask> >& tasks = it->second;
        if (tasks.size())
        {
            for (std::vector<boost::shared_ptr<const baseTask> >::const_iterator it2 = tasks.begin(); it2 != tasks.end(); ++it2)
            {
                cData.m_trunk_tasks.acceptTask(*it2);
            }
            return HC_SUCCESS;
        }
        ++it;
    }
    return HC_ERROR;
}


