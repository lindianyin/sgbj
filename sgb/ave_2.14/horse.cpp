#include "statistics.h"

#include "horse.h"
#include "net.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include "utils_lang.h"
#include "daily_task.h"
#include "spls_timer.h"
#include "singleton.h"
#include "relation.h"
#include "new_event.h"

extern void InsertSaveDb(const std::string& sql);
extern Database& GetDb();

//战马链接
const std::string strHorseLink = "<A HREF=\"event:{'cmd':'showHorse','cid':$C}\" TARGET=\"\"><U>$N</U></A>";

static const int iDefaultHorseTrainTime = 20;
static const int iDefaultHorseGoldTrainTime = 3;

volatile int iHorseTrainTime = iDefaultHorseTrainTime;        //每天培养战马次数
volatile int iHorseGoldTrainTime = iDefaultHorseGoldTrainTime;    //每天金币培养战马次数

//战马训练折扣
volatile int iHorseTrainDiscount = 100;

using namespace std;
using namespace net;

//static     boost::uuids::nil_generator gen;

int CharHorseFruit::start()
{
    if (!fruit.get() || (state != 1 && state != 2))
    {
        return HC_ERROR;
    }
    //重启后的情况
    int leftsecond = end_time - time(NULL);

    json_spirit::mObject mobj;
    mobj["cmd"] = "fruitDone";
    mobj["cid"] = cid;
    mobj["id"] = fruit->id;
    boost::shared_ptr<splsTimer> tmsg;
    if (leftsecond <= 0)
    {
        //直接完成了
        tmsg.reset(new splsTimer(0.1, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    else
    {
        tmsg.reset(new splsTimer(leftsecond, 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }
    return 0;
}

int CharHorseFruit::stop()
{
    if (start_time != 0 && end_time != 0 && state == 3)
    {
        splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int CharHorse::save()
{
    InsertSaveDb("replace into char_horses (cid,horseid,exp,action_start,action_end,pugong,cegong,pufang,cefang,bingli) values (" + LEX_CAST_STR(cid)
        + "," + LEX_CAST_STR(horseid)
        + "," + LEX_CAST_STR(exp)
        + "," + LEX_CAST_STR(start_time)
        + "," + LEX_CAST_STR(end_time)
        + "," + LEX_CAST_STR(pugong)
        + "," + LEX_CAST_STR(pufang)
        + "," + LEX_CAST_STR(cegong)
        + "," + LEX_CAST_STR(cefang)
        + "," + LEX_CAST_STR(bingli)
        + ")");
    return 0;
}

int CharHorse::save_action()
{
#if 1
    //战马活动取消
    return 0;
#else
    InsertSaveDb("delete from char_horses_action where cid=" + LEX_CAST_STR(cid));
    for (size_t horse_i = 0; horse_i < action_list.size(); ++horse_i)
    {
        for (size_t i = 0; i < action_list[horse_i].fruits_list.size(); ++i)
        {
            if (action_list[horse_i].fruits_list[i].fruit.get())
            {
                InsertSaveDb("insert into char_horses_action (cid,horse_id,fruit_id,state,start_time,end_time) values (" + LEX_CAST_STR(cid)
                    + "," + LEX_CAST_STR(action_list[horse_i].horse_id)
                    + "," + LEX_CAST_STR(action_list[horse_i].fruits_list[i].fruit->id)
                    + "," + LEX_CAST_STR(action_list[horse_i].fruits_list[i].state)
                    + "," + LEX_CAST_STR(action_list[horse_i].fruits_list[i].start_time)
                    + "," + LEX_CAST_STR(action_list[horse_i].fruits_list[i].end_time)
                    + ")");
            }
        }
    }
    return 0;
#endif
}

int CharHorse::checkFruitState()
{
#if 1
    //战马活动取消
    return 0;
#else
    if (end_time < time(NULL))
        return 0;
    bool needsave = false;
    if (action_list.empty() || action_list[0].fruits_list.empty())
        return 0;
    for (size_t i = 0; i < action_list[0].fruits_list.size(); ++i)
    {
        if (action_list[0].fruits_list[i].fruit.get()
            && action_list[0].fruits_list[i].fruit->eat_horseid <= horseid
            &&  action_list[0].fruits_list[i].state == 1)
        {
            action_list[0].fruits_list[i].state = 2;
            needsave = true;
        }
    }
    if (needsave)
    {
        save_action();
    }
    return 0;
#endif
}

int CharHorse::updateActionFruit()
{
#if 1
    //战马活动取消
    return 0;
#else
    int id = 0;
    if (!action_list.empty())
    {
        for (size_t i = 0; i < action_list[0].fruits_list.size(); ++i)
        {
            //第一只马三个果子还有效则不改变活动马
            if (action_list[0].fruits_list[i].state == 0 || action_list[0].fruits_list[i].state == 1 || action_list[0].fruits_list[i].state == 2)
            {
                return 0;
            }
        }
        id = action_list[0].horse_id;
    }
    action_list.clear();
    for(int i = 0; i < iMaxAction; ++i)
    {
        if (id == 0 && horseid == iMaxHorse)
        {
            return 0;
        }
        id = horseMgr::getInstance()->getNextActionHorse(id);
        if (id != 0)
        {
            CharHorseFruitAction chfa(cid,id);
            baseHorse* p = horseMgr::getInstance()->getHorse(id);
            if (p)
            {
                chfa.horse_star = p->star;
                chfa.horse_name = p->name;
                if (p->fruit[0])
                {
                    boost::shared_ptr<baseHorseFruit> pbh = horseMgr::getInstance()->getBaseHorseFruit(p->fruit[0]);
                    if (pbh.get())
                    {
                        CharHorseFruit chf(cid);
                        chf.fruit = pbh;
                        chfa.fruits_list.push_back(chf);
                    }
                }
                if (p->fruit[1])
                {
                    boost::shared_ptr<baseHorseFruit> pbh = horseMgr::getInstance()->getBaseHorseFruit(p->fruit[1]);
                    if (pbh.get())
                    {
                        CharHorseFruit chf(cid);
                        chf.fruit = pbh;
                        chfa.fruits_list.push_back(chf);
                    }
                }
                if (p->fruit[2])
                {
                    boost::shared_ptr<baseHorseFruit> pbh = horseMgr::getInstance()->getBaseHorseFruit(p->fruit[2]);
                    if (pbh.get())
                    {
                        CharHorseFruit chf(cid);
                        chf.fruit = pbh;
                        chfa.fruits_list.push_back(chf);
                    }
                }
                action_list.push_back(chfa);
            }
        }
    }
    save_action();
    return 0;
#endif
}

void CharHorse::updateNewAttack()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!cdata.get())
        return;
    if (horse)
    {
        _score = 0;
        _power = 0;
        int tmp = horse->star + horse->turn * 10;
        if (cdata->m_level < 50)
        {
            _score = tmp * 4;
        }
        else if(cdata->m_level < 70)
        {
            _score = tmp * 2;
        }
        else
        {
            _score = tmp * 5 / 3;
        }
        _score = _score > 100 ? 100 : _score;
        _power = cdata->buff_attack(act_wuli_attack, horse->pugong, horse->bingli, horse->pufang, horse->cefang);
    }
    return;
}

horseMgr* horseMgr::m_handle = NULL;
horseMgr* horseMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new horseMgr();
        m_handle->reload();
    }
    return m_handle;
}

baseHorse* horseMgr::getHorse(int id)
{
    if (id >= 1 && id <= iMaxHorse)
    {
        return base_horses + id - 1;
    }
    return NULL;
}

int horseMgr::getHorseInfo(CharData& cData, json_spirit::Object& robj)
{
    if (!cData.m_horse.horse)
        return HC_ERROR;
    json_spirit::Object info;
    if (cData.m_horse.horse)
    {
        info.push_back( Pair("id", cData.m_horse.horse->id) );
        info.push_back( Pair("name", cData.m_horse.horse->name) );
        info.push_back( Pair("quality", cData.m_horse.horse->quality) );
        info.push_back( Pair("spic", cData.m_horse.horse->spic) );
        info.push_back( Pair("stars", cData.m_horse.horse->star) );
        info.push_back( Pair("turns", cData.m_horse.horse->turn) );
        info.push_back( Pair("curExp", cData.m_horse.exp) );
        baseHorse* next_horse = getHorse(cData.m_horse.horse->id + 1);
        if (next_horse != NULL)
            info.push_back( Pair("totalExp", next_horse->need_exp) );
        json_spirit::Object curAttr;
        curAttr.push_back( Pair("atk_normal", cData.m_horse.horse->pugong) );
        curAttr.push_back( Pair("def_normal", cData.m_horse.horse->pufang) );
        curAttr.push_back( Pair("atk_tactics", cData.m_horse.horse->cegong) );
        curAttr.push_back( Pair("def_tactics", cData.m_horse.horse->cefang) );
        curAttr.push_back( Pair("army", cData.m_horse.horse->bingli) );
        info.push_back( Pair("curAttr", curAttr) );
        json_spirit::Object fruit;
        fruit.push_back( Pair("atk_normal", cData.m_horse.pugong) );
        fruit.push_back( Pair("def_normal", cData.m_horse.pufang) );
        fruit.push_back( Pair("atk_tactics", cData.m_horse.cegong) );
        fruit.push_back( Pair("def_tactics", cData.m_horse.cefang) );
        fruit.push_back( Pair("army", cData.m_horse.bingli) );
        info.push_back( Pair("fruit", fruit) );
        json_spirit::Object nextAttr;
        baseHorse* p = getHorse(cData.m_horse.horse->id + 1);
        if (p != NULL)
        {
            nextAttr.push_back( Pair("atk_normal", p->pugong) );
            nextAttr.push_back( Pair("def_normal", p->pufang) );
            nextAttr.push_back( Pair("atk_tactics", p->cegong) );
            nextAttr.push_back( Pair("def_tactics", p->cefang) );
            nextAttr.push_back( Pair("army", p->bingli) );
        }
        info.push_back( Pair("nextAttr", nextAttr) );

        json_spirit::Object extra;
        int total_left = iHorseTrainTime - cData.m_silver_train_horse;
        if (total_left > 0)
        {
            if (iHorseTrainDiscount == 100)
            {
                extra.push_back( Pair("silverPrice", 3000*cData.m_horse.horse->turn + 3000) );
            }
            else
            {
                extra.push_back( Pair("silverPrice", 30*(cData.m_horse.horse->turn + 1)*iHorseTrainDiscount) );
            }
            extra.push_back( Pair("silverTrainLeft", total_left) );
        }
        int cost_gold = iHorseTrainGold;
        if (cData.m_gold_train_horse / 3 > 0)
            cost_gold += (cData.m_gold_train_horse / 3 * 50);
        if (cost_gold > 500)
            cost_gold = 500;

        if (iHorseTrainDiscount != 100)
        {
            cost_gold = cost_gold * iHorseTrainDiscount/100;
        }
        extra.push_back( Pair("goldPrice", cost_gold) );

        extra.push_back( Pair("matitie", cData.treasureCount(treasure_type_mati_tie)) );
        if (next_horse != NULL)
            extra.push_back( Pair("prestige", next_horse->need_prestige) );
        extra.push_back( Pair("mati_ling", cData.m_bag.getGemCount(treasure_type_mati_tie)) );
        info.push_back( Pair("extra", extra) );

        info.push_back( Pair("horseAction", cData.m_horse.end_time > time(NULL) ? 1 : 0) );
    }
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

int horseMgr::getHorseFruitsList(CharData& cData, json_spirit::Object& robj)
{
#if 1
    //战马活动取消
    return 0;
#else
    if (!cData.m_horse.horse)
        return HC_ERROR;
    if (cData.m_horse.end_time < time(NULL))
    {
        return HC_ERROR;
    }
    json_spirit::Array list;
    if (cData.m_horse.horse)
    {
        for (size_t horse_i = 0; horse_i < cData.m_horse.action_list.size(); ++horse_i)
        {
            json_spirit::Object horse;
            json_spirit::Array fruit_list;
            for (size_t i = 0; i < cData.m_horse.action_list[horse_i].fruits_list.size(); ++i)
            {
                if (cData.m_horse.action_list[horse_i].fruits_list[i].fruit.get())
                {
                    json_spirit::Object fruit;
                    fruit.push_back( Pair("id", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->id) );
                    fruit.push_back( Pair("type", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->type) );
                    fruit.push_back( Pair("name", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->name) );
                    fruit.push_back( Pair("state", cData.m_horse.action_list[horse_i].fruits_list[i].state) );
                    if (cData.m_horse.action_list[horse_i].fruits_list[i].state == 2 || cData.m_horse.action_list[horse_i].fruits_list[i].state == 1)
                    {
                        fruit.push_back( Pair("leftTime", cData.m_horse.action_list[horse_i].fruits_list[i].end_time - time(NULL)) );
                    }
                    baseHorse* p = getHorse(cData.m_horse.action_list[horse_i].fruits_list[i].fruit->eat_horseid);
                    if (p)
                    {
                        fruit.push_back( Pair("star", p->star) );
                    }
                    json_spirit::Object powerVO;
                    powerVO.push_back( Pair("atk_normal", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->pugong) );
                    powerVO.push_back( Pair("def_normal", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->pufang) );
                    powerVO.push_back( Pair("atk_tactics", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->cegong) );
                    powerVO.push_back( Pair("def_tactics", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->cefang) );
                    powerVO.push_back( Pair("army", cData.m_horse.action_list[horse_i].fruits_list[i].fruit->bingli) );
                    fruit.push_back( Pair("powerVO", powerVO) );
                    fruit_list.push_back(fruit);
                }
            }
            horse.push_back( Pair("canGet", cData.m_horse.horse->id >= cData.m_horse.action_list[horse_i].horse_id) );
            horse.push_back( Pair("horse_name", cData.m_horse.action_list[horse_i].horse_name) );
            horse.push_back( Pair("horse_star", cData.m_horse.action_list[horse_i].horse_star) );
            horse.push_back( Pair("horse_id", cData.m_horse.action_list[horse_i].horse_id) );
            horse.push_back( Pair("fruit_list", fruit_list) );
            list.push_back(horse);
        }
    }
    robj.push_back( Pair("list", list) );
    json_spirit::Object info;
    info.push_back( Pair("startTime", cData.m_horse.start_time) );
    info.push_back( Pair("endTime", cData.m_horse.end_time) );
    //info.push_back( Pair("leftTime", cData.m_horse.end_time - time(NULL)) );
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
#endif
}

std::string horseMgr::NameToLink(int cid, std::string name, int quality)
{
    std::string result = strHorseLink;
    str_replace(result, "$C", LEX_CAST_STR(cid));
    str_replace(result, "$N", name);
    addColor(result, quality);
    return result;
}

int horseMgr::turnHorse(CharData& cData, json_spirit::Object& robj)
{
    if (cData.m_horse.horse == NULL)
        return HC_ERROR;
    if (cData.m_horse.horse->star < iHorseStarsMax)
        return HC_ERROR;
    baseHorse* next_horse = getHorse(cData.m_horse.horse->id + 1);
    if (next_horse == NULL)
        return HC_ERROR;
    if (cData.m_prestige < next_horse->need_prestige)
        return HC_ERROR_NEED_MORE_PRESTIGE;
    if (cData.m_horse.horse->turn < iHorseTurnsMax)
    {
        std::string msg = strHorseTurn;
        str_replace(msg, "$P", cData.m_horse.horse->name);
        str_replace(msg, "$N", next_horse->name);
        robj.push_back( Pair("msg", msg) );
        //公告
        std::string notify_msg = "";
        notify_msg = strHorseTurnMsg;
        str_replace(notify_msg, "$N", MakeCharNameLink(cData.m_name));
        str_replace(notify_msg, "$H", NameToLink(cData.m_id,cData.m_horse.horse->name,cData.m_horse.horse->quality));
        str_replace(notify_msg, "$h", NameToLink(cData.m_id,next_horse->name,next_horse->quality));
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
        cData.m_horse.horse = next_horse;
        cData.m_horse.exp = 0;
        ++cData.m_horse.horseid;

        cData.set_attack_change();
        cData.m_horse.updateNewAttack();

        cData.m_horse.save();

        if (cData.m_horse.horse->turn >= 2)
        {
            Singleton<relationMgr>::Instance().postCongradulation(cData.m_id, cData.m_horse.horse->turn+1, cData.m_horse.horseid, 0);
        }
    }
    return HC_SUCCESS;
}

int horseMgr::trainHorse(CharData& cData, int type, int& cri_type, int& get_exp, int& dj_use)
{
    if (cData.m_horse.horse == NULL)
        return HC_ERROR;
    if (cData.m_horse.horse->star >= iHorseStarsMax)
        return HC_ERROR;
    baseHorse* next_horse = getHorse(cData.m_horse.horse->id + 1);
    if (next_horse == NULL)
        return HC_ERROR;

    cri_type = 0;
    get_exp = cData.m_horse.horse->add_exp_silver;

    dj_use = 0;
    int pre_horse_id = cData.m_horse.horseid;
    switch (type)
    {
        case 2://silver
            {
                if (cData.m_silver_train_horse >= iHorseTrainTime)
                {
                    return HC_ERROR_NOT_ENOUGH_TIME;
                }
                int silver = 3000 * (cData.m_horse.horse->turn + 1);
                if (iHorseTrainDiscount != 100)
                {
                    silver = silver * iHorseTrainDiscount/100;
                }
                if (cData.addSilver(-silver) < 0)
                    return HC_ERROR_NOT_ENOUGH_SILVER;
                //银币消耗统计
                add_statistics_of_silver_cost(cData.m_id, cData.m_ip_address, silver, silver_cost_for_horse_train, cData.m_union_id, cData.m_server_id);
                if (my_random(0,100) < 15)
                {
                    cri_type = 1;    //小暴击
                    cData.m_horse.exp += cData.m_horse.horse->add_exp_silver * 10;
                }
                else
                {
                    cData.m_horse.exp += cData.m_horse.horse->add_exp_silver;
                    ++cData.m_silver_train_horse;
                    cData.setExtraData(char_data_type_daily, char_data_horse_silver_train, cData.m_silver_train_horse);
                }
                while (cData.m_horse.exp >= next_horse->need_exp && cData.m_horse.horse->star < iHorseStarsMax)
                {
                    cData.m_horse.exp -= next_horse->need_exp;
                    cData.m_horse.horse = next_horse;
                    ++cData.m_horse.horseid;
                    cData.m_horse.checkFruitState();
                    next_horse = getHorse(cData.m_horse.horse->id + 1);
                    if (next_horse == NULL)
                        break;
                }
                cData.m_horse.save();
            //act统计
            act_to_tencent(&cData,act_new_horse_train_silver);
            }
            break;
        case 3://gold
            {
                //优先扣道具
                if (cData.addTreasure(treasure_type_mati_tie, -1) < 0)
                {
                    int cost_gold = iHorseTrainGold;
                    if (cData.m_gold_train_horse / 3 > 0)
                        cost_gold += (cData.m_gold_train_horse / 3 * 50);
                    if (cost_gold > 500)
                        cost_gold = 500;

                    if (iHorseTrainDiscount != 100)
                    {
                        cost_gold = cost_gold * iHorseTrainDiscount/100;
                    }
                    if (cData.addGold(-cost_gold) == -1)
                        return HC_ERROR_NOT_ENOUGH_GOLD;
                    //金币消耗统计
                    add_statistics_of_gold_cost(cData.m_id, cData.m_ip_address, cost_gold, gold_cost_for_horse_train, cData.m_union_id, cData.m_server_id);
#ifdef QQ_PLAT
                    gold_cost_tencent(&cData,cost_gold,gold_cost_for_horse_train);
#endif
                }
                else
                {
                    dj_use = 1;
                }
                if (my_random(1,100) <= 2)
                {
                    cri_type = 2;    //大暴击
                    cData.m_horse.horse = next_horse;
                    cData.m_horse.exp = 0;
                    ++cData.m_horse.horseid;
                    cData.m_horse.checkFruitState();
                }
                else
                {
                    cri_type = 1;    //小暴击
                    cData.m_horse.exp += cData.m_horse.horse->add_exp_silver * 10;

                    while (cData.m_horse.exp >= next_horse->need_exp && cData.m_horse.horse->star < iHorseStarsMax)
                    {
                        cData.m_horse.exp -= next_horse->need_exp;
                        cData.m_horse.horse = next_horse;
                        ++cData.m_horse.horseid;
                        cData.m_horse.checkFruitState();
                        next_horse = getHorse(cData.m_horse.horse->id + 1);
                        if (next_horse == NULL)
                            break;
                    }
                }
                cData.m_horse.save();

                ++cData.m_gold_train_horse;
                cData.setExtraData(char_data_type_daily, char_data_horse_gold_train, cData.m_gold_train_horse);
            }
            break;
        default:
            return HC_ERROR;
    }
    if (pre_horse_id != cData.m_horse.horseid)
    {
        cData.set_attack_change();
        cData.m_horse.updateNewAttack();
    }
    if (cData.m_horse.horse->star == iHorseStarsMax)
        cData.m_horse.exp = 0;
    #if 0
    if((cData.m_silver_train_horse + cData.m_gold_train_horse) >= (iHorseTrainTime))
    {
        json_spirit::Object obj;
        obj.push_back( Pair("cmd", "getUpdateList") );
        obj.push_back( Pair("s", 200) );
        cData.getUpdateListCD(obj);
        cData.sendObj(obj);
    }
    #endif

    if (2 == type)
    {
        //支线任务
        cData.m_trunk_tasks.updateTask(task_horse_train, 1);
    }
    return HC_SUCCESS;
}

//设置战马等级
int horseMgr::setHorse(CharData& cData, int horseId, int exp)
{
    if (cData.m_horse.horse == NULL)
    {
        return HC_ERROR;
    }
    baseHorse* next_horse = getHorse(horseId);
    if (next_horse == NULL)
        return HC_ERROR;
    cData.m_horse.horse = next_horse;
    cData.m_horse.horseid = horseId;
    cData.m_horse.exp = exp;
    cData.m_horse.save();
    return HC_SUCCESS;
}

int horseMgr::fruitDone(int cid, int id)
{
    boost::shared_ptr<CharData> pcd = GeneralDataMgr::getInstance()->GetCharData(cid);
    if (!pcd.get())
        return HC_ERROR;
    if (pcd->m_horse.horse == NULL)
    {
        return HC_ERROR;
    }
    for (size_t i = 0; i < pcd->m_horse.action_list[0].fruits_list.size(); ++i)
    {
        if (pcd->m_horse.action_list[0].fruits_list[i].fruit.get()
            && pcd->m_horse.action_list[0].fruits_list[i].fruit->id == id
            && (pcd->m_horse.action_list[0].fruits_list[i].state == 1 || pcd->m_horse.action_list[0].fruits_list[i].state == 2))
        {
            pcd->m_horse.action_list[0].fruits_list[i].state = 4;
            pcd->m_horse.updateActionFruit();
            pcd->m_horse.save_action();
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

int horseMgr::dealHorseFruit(CharData& cData, int type, int id)
{
    //战马活动取消
    return 0;
    if (cData.m_horse.horse == NULL || cData.m_horse.action_list.empty())
    {
        return HC_ERROR;
    }
    if (cData.m_horse.end_time < time(NULL))
    {
        return HC_ERROR;
    }
    if (type == 1)//领取
    {
        //只能领取第一只战马果子
        if (cData.m_horse.action_list[0].horse_id == id)
        {
            for (size_t i = 0; i < cData.m_horse.action_list[0].fruits_list.size(); ++i)
            {
                if (cData.m_horse.action_list[0].fruits_list[i].fruit.get()
                    && cData.m_horse.action_list[0].fruits_list[i].fruit->get_horseid <= cData.m_horse.horseid
                    && cData.m_horse.action_list[0].fruits_list[i].state == 0)
                {
                    if (cData.m_horse.action_list[0].fruits_list[i].fruit->eat_horseid <= cData.m_horse.horseid)
                        cData.m_horse.action_list[0].fruits_list[i].state = 2;
                    else
                        cData.m_horse.action_list[0].fruits_list[i].state = 1;
                    cData.m_horse.action_list[0].fruits_list[i].start_time = time(NULL);
                    cData.m_horse.action_list[0].fruits_list[i].end_time = cData.m_horse.action_list[0].fruits_list[i].start_time + iHorseFruitTime;
                    cData.m_horse.action_list[0].fruits_list[i].start();
                }
            }
            cData.m_horse.save_action();
            return HC_SUCCESS;
        }
    }
    else if (type == 2)//服用
    {
        //只能服用第一只战马果子
        for (size_t i = 0; i < cData.m_horse.action_list[0].fruits_list.size(); ++i)
        {
            if (cData.m_horse.action_list[0].fruits_list[i].fruit.get()
                && cData.m_horse.action_list[0].fruits_list[i].fruit->id == id
                && cData.m_horse.action_list[0].fruits_list[i].fruit->eat_horseid <= cData.m_horse.horseid
                && cData.m_horse.action_list[0].fruits_list[i].state == 2)
            {
                cData.m_horse.action_list[0].fruits_list[i].state = 3;
                cData.m_horse.action_list[0].fruits_list[i].stop();
                cData.m_horse.pugong += cData.m_horse.action_list[0].fruits_list[i].fruit->pugong;
                cData.m_horse.cegong += cData.m_horse.action_list[0].fruits_list[i].fruit->cegong;
                cData.m_horse.pufang += cData.m_horse.action_list[0].fruits_list[i].fruit->pufang;
                cData.m_horse.cefang += cData.m_horse.action_list[0].fruits_list[i].fruit->cefang;
                cData.m_horse.bingli += cData.m_horse.action_list[0].fruits_list[i].fruit->bingli;
                cData.m_horse.updateActionFruit();
                cData.m_horse.save();
                cData.m_horse.save_action();
                return HC_SUCCESS;
            }
        }
    }
    return HC_ERROR;
}

int horseMgr::getNextActionHorse(int horseid)
{
    std::list<int>::iterator it = HorseFruitAction.begin();
    while(it != HorseFruitAction.end())
    {
        if (*it > horseid)
        {
            return *it;
        }
        ++it;
    }
    return 0;
}

boost::shared_ptr<baseHorseFruit> horseMgr::getBaseHorseFruit(int fruit_id)
{
    boost::shared_ptr<baseHorseFruit> pbhf;
    pbhf.reset();
    std::map<int,boost::shared_ptr<baseHorseFruit> >::iterator it = base_fruits_list.find(fruit_id);
    if (it != base_fruits_list.end())
    {
        pbhf = it->second;
    }
    return pbhf;
}

int horseMgr::reload()
{
    Query q(GetDb());
    q.get_result("select id,level,star,turn,name,spic,pugong,pufang,cegong,cefang,bingli,need_exp,silver_exp,gold_exp,need_prestige,fruit_id1,fruit_id2,fruit_id3 from base_horses where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int horseid = q.getval();
        if (horseid > iMaxHorse || horseid < 1)
        {
            ERR();
            continue;
        }
        base_horses[horseid-1].id = horseid;
        base_horses[horseid-1].level = q.getval();
        base_horses[horseid-1].star = q.getval();
        base_horses[horseid-1].turn = q.getval();
        base_horses[horseid-1].name = q.getstr();
        base_horses[horseid-1].spic = q.getval();
        base_horses[horseid-1].pugong = q.getval();
        base_horses[horseid-1].pufang= q.getval();
        base_horses[horseid-1].cegong = q.getval();
        base_horses[horseid-1].cefang = q.getval();
        base_horses[horseid-1].bingli = q.getval();
        base_horses[horseid-1].need_exp= q.getval();
        base_horses[horseid-1].add_exp_silver = q.getval();
        base_horses[horseid-1].add_exp_gold= q.getval();
        base_horses[horseid-1].need_prestige = q.getval();
        base_horses[horseid-1].fruit[0] = q.getval();
        base_horses[horseid-1].fruit[1] = q.getval();
        base_horses[horseid-1].fruit[2] = q.getval();
        if (base_horses[horseid-1].fruit[0] != 0)
        {
            HorseFruitAction.push_back(horseid);
        }
        base_horses[horseid-1].quality = iHorseQuality[base_horses[horseid-1].turn];
    }
    q.free_result();
    q.get_result("select id,name,type,pugong,cegong,pufang,cefang,bingli,get_horse,eat_horse from base_horses_fruits where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<baseHorseFruit> pbhf;
        pbhf.reset(new baseHorseFruit);
        int id = q.getval();
        pbhf->id = id;
        pbhf->name = q.getstr();
        pbhf->type = q.getval();
        pbhf->pugong = q.getval();
        pbhf->cegong = q.getval();
        pbhf->pufang = q.getval();
        pbhf->cefang = q.getval();
        pbhf->bingli = q.getval();
        pbhf->get_horseid = q.getval();
        pbhf->eat_horseid = q.getval();
        base_fruits_list[id] = pbhf;
    }
    q.free_result();

    iHorseGoldTrainTime = GeneralDataMgr::getInstance()->getInt("horse_train_gold_t", iDefaultHorseGoldTrainTime);
    iHorseTrainTime = GeneralDataMgr::getInstance()->getInt("horse_train_total_t", iDefaultHorseTrainTime);
    iHorseTrainDiscount = GeneralDataMgr::getInstance()->getInt("horse_discount", 100);
    if (iHorseTrainDiscount <= 0)
    {
        iHorseTrainDiscount = 100;
    }
    return 0;
}

void horseMgr::setHorseTrainTimes(int total, int gold_times)
{
    if (total <= 0 && gold_times <= 0)
    {
        iHorseTrainTime = iDefaultHorseTrainTime;        //每天培养战马次数
        iHorseGoldTrainTime = iDefaultHorseGoldTrainTime;    //每天金币培养战马次数
    }
    else
    {
        iHorseTrainTime = total;
        iHorseGoldTrainTime = gold_times;
    }

    GeneralDataMgr::getInstance()->setInt("horse_train_total_t", iHorseTrainTime);
    GeneralDataMgr::getInstance()->setInt("horse_train_gold_t", iHorseGoldTrainTime);
}

