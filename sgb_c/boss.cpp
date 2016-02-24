
#include "boss.h"
#include "utils_all.h"
#include "errcode_def.h"
#include "const_def.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "data.h"
#include "spls_timer.h"
#include "net.h"
#include "utils_lang.h"
#include <syslog.h>
#include "statistics.h"
#include "singleton.h"
#include "SaveDb.h"
#include "rewards.h"

using namespace json_spirit;
using namespace net;

extern std::string strBossWillOpen;
extern std::string strBossOpening;
extern std::string strBossIsClosed;
extern std::string strBossKillMsg;
extern std::string strBossTop1Damage;
extern std::string strBossTop2Damage;
extern std::string strBossTop3Damage;
extern std::string strBossMail;
extern std::string strBossKillMail;
extern std::string strBossLog;
extern std::string strBossMailTitle;
extern std::string strBossOpenMsg;
extern std::string strBossDyingMsg;
extern std::string strBossGet300KMsg;

const std::string strNotifyBossHp = "{\"cmd\":\"getBossHp\",\"id\":$B,\"s\":200,\"hp\":$H,\"hide_hp\":$h,\"special\":$S,\"attacker\":\"$A\"}";
const std::string strBossActionEnd = "{\"cmd\":\"boss_end\",\"id\":$B,\"s\":200}";
const std::string strNotifyBossOpen = "{\"cmd\":\"updateTopButton\",\"type\":"+LEX_CAST_STR(top_button_dailyAction)+",\"s\":200,\"active\":1}";
const std::string strNotifyBossClose = "{\"cmd\":\"updateTopButton\",\"type\":"+LEX_CAST_STR(top_button_dailyAction)+",\"s\":200,\"active\":0}";
const std::string strNotifyBossDying = "{\"cmd\":\"boss_dying\",\"name\":\"$B\",\"s\":200}";

#define INFO(x)

Database& GetDb();

bool compare_score(bossScore& a, bossScore& b)
{
    return a.score > b.score;
}

baseBossData::baseBossData()
{
    //openRules _open_rules;

    _open_state = 0;
    _id = 0;
    _spic = 0;
    _attack = 0;
    _defense = 0;
    _magic = 0;
    _max_hp = 0;
    _cur_hp = 0;
    _hide_hp = 0;
    _level = 0;
    _base_attack = 0;
    _base_defense = 0;
    _base_magic = 0;
    _base_hp = 0;
    _attack_per_level = 0;
    _defense_per_level = 0;
    _magic_per_level = 0;
    _hp_per_level = 0;
    _name = "";
    _chat = "";
    _last_open_time = 0;
}

Boss::Boss(int id)
{
    //memset(&_boss, 0, sizeof(bossData));
    _boss._id = id;
    _start_time = 0;
    _end_time = 0;
    _uuid = boost::uuids::nil_uuid();

    _boss_hp_msg = strNotifyBossHp;
    str_replace(_boss_hp_msg, "$B", LEX_CAST_STR(_boss._id));

    _channel.reset(new ChatChannel("boss", id, "{\"cmd\":\"chat\",\"ctype\":100,\"s\":200,"));
    _channel->start();
}

//是否能攻击
int Boss::canAttack(int cid, int& cool)
{
    if (_boss._open_state == 0)
    {
        return HC_ERROR_BOSS_EVENT_END;
    }
    if (time(NULL) - _start_time < iBossCombatDelay)
    {
        return HC_ERROR_BOSS_WAITING;
    }
    if (m_char_list.find(cid) == m_char_list.end())
    {
        return HC_ERROR_BOSS_NOT_ENTER;
    }
    int cd = m_attack_cd_maps[cid] - time(NULL);
    if (cd > 0)
    {
        cool = cd;
        INFO("coolTime "<<m_attack_cd_maps[cid]<<" > "<<time(NULL)<<endl);
        return HC_ERROR_IN_COOLTIME;
    }
    return HC_SUCCESS;
}

void Boss::reset()
{
    _boss._cur_hp = _boss._max_hp;
    _boss._hide_hp = _boss._max_hp / 20;
    _start_time = 0;
    _end_time = 0;
    return;
}

int Boss::combatResult(chessCombat* pCombat)    //boss战斗结束
{
    if (_boss._open_state == 0)
    {
        return HC_SUCCESS;
    }
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (COMBAT_TYPE_BOSS != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    int damage = pCombat->m_players[1].m_hp_org - pCombat->m_players[1].m_hp_left;
    _boss._cur_hp -= damage;
    //BOSS奄奄一息
    if (_boss._cur_hp + damage > _boss._hide_hp && _boss._cur_hp <= _boss._hide_hp)
    {
        std::string msg = strNotifyBossDying;
        str_replace(msg, "$B", _boss._name);
        GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(msg, iBossOpenLevel);

        msg = strBossDyingMsg;
        str_replace(msg, "$B", _boss._name);
        str_replace(msg, "$n", LEX_CAST_STR(_boss._id));
        str_replace(msg, "$S", LEX_CAST_STR(_boss._spic));
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }

    if (_boss._cur_hp <= 0)
        _boss._cur_hp = 0;

    //广播boss血量
    {
        std::string hpmsg = _boss_hp_msg;
        if (_boss._cur_hp <= _boss._hide_hp)
        {
            str_replace(hpmsg, "$h", LEX_CAST_STR(_boss._hide_hp));
            str_replace(hpmsg, "$H", LEX_CAST_STR(_boss._cur_hp));
            str_replace(hpmsg, "$S", "true");
        }
        else
        {
            str_replace(hpmsg, "$h", LEX_CAST_STR(0));
            str_replace(hpmsg, "$H", LEX_CAST_STR(_boss._cur_hp));
            str_replace(hpmsg, "$S", "false");
        }
        str_replace(hpmsg, "$A", pCombat->m_players[0].m_player_name);
        _channel->BroadMsg(hpmsg);
    }

    int acid = pCombat->m_players[0].m_cid;
    m_damage_maps[acid] += damage;
    m_score_maps[acid] = m_damage_maps[acid] * 60 / 100 / 100 + m_silver_maps[acid] * 40 / 100 / 1000;

    //记录到信息
    if (damage > 0)
    {
        boost::shared_ptr<boss_log> p;
        p.reset(new boss_log());
        p->m_name = pCombat->m_players[0].m_player_name;
        p->m_damage = damage;
        if (m_event_log.size() >= 5)
            m_event_log.pop_front();
        m_event_log.push_back(p);
        //广播给其他人
        broadBossLogEvent();
    }

    int rank = 0;
    bool find = false;
    bool bTop10_change = false;
    for (std::list<bossScore>::iterator it = _topten.begin(); it != _topten.end(); ++it)
    {
        ++rank;
        if ((*it).cid == acid)
        {
            (*it).score = m_score_maps[acid];
            INFO(" damage "<<damage<<",total score "<<it->score<<endl);
            find = true;
            break;
        }
    }
    if (!find)
    {
        bossScore bossd;
        bossd.cid = acid;
        bossd.score = m_score_maps[bossd.cid];
        bossd.name = pCombat->m_players[0].m_player_name;
        INFO(" damage "<<damage<<",total score "<<bossd.score<<endl);
        _topten.push_back(bossd);
    }

    _topten.sort(compare_score);

    if (rank <= 10)
    {
        bTop10_change = true;
    }
    else
    {
        int new_rank = 1;
        for (std::list<bossScore>::iterator it = _topten.begin(); it != _topten.end() && new_rank <= 10; ++it, ++new_rank)
        {
            if (it->cid == acid)
            {
                bTop10_change = true;
                break;
            }
        }
    }
    //前10名信息发生变化,广播
    if (bTop10_change)
    {
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "getBossRank") );
        robj.push_back( Pair("id", _boss._id) );
        robj.push_back( Pair("s", 200) );
        json_spirit::Array dlist;
        int rank = 0;
        for (std::list<bossScore>::iterator it = _topten.begin(); it != _topten.end(); ++it)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("rank", ++rank) );
            obj.push_back( Pair("name", it->name) );
            obj.push_back( Pair("score", it->score) );
            dlist.push_back(obj);
            if (rank >= 10)
            {
                break;
            }
        }
        robj.push_back( Pair("list", dlist) );
        INFO("#################### boss combat broad top 10 #########################"<<endl);
        _channel->BroadMsg(write(robj, json_spirit::raw_utf8));
    }

    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(acid);
    if (cdata.get())
    {
        //冷却时间50秒
        m_attack_cd_maps[acid] = time(NULL) + iBossCombatCD;

        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "getBossRank") );
        robj.push_back( Pair("id", _boss._id) );
        robj.push_back( Pair("s", 200) );
        robj.push_back( Pair("silver", m_silver_maps[acid]) );
        robj.push_back( Pair("damage", m_damage_maps[acid]) );
        robj.push_back( Pair("score", m_score_maps[acid]) );
        cdata->sendObj(robj);

        cdata->m_tasks.updateTask(GOAL_DAILY_BOSS, 0, 1);
#if 0
        //筹码
        if (pCombat->m_players[0].m_silver < 0)
            pCombat->m_players[0].m_silver = 0;
        cdata->silver(pCombat->m_players[0].m_silver);
        //silver(int)没有处理统计
        if (pCombat->m_players[0].m_silver > pCombat->m_players[0].m_org_silver)
        {
            int total = pCombat->m_players[0].m_silver - pCombat->m_players[0].m_org_silver;
            if (total > 300000)
            {
        		std::string msg = strBossGet300KMsg;
    			str_replace(msg, "$W", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
    			str_replace(msg, "$G", LEX_CAST_STR(total));
        		if (msg != "")
        		{
        			GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        		}
            }
            statistics_of_silver_get(cdata->m_id, cdata->m_ip_address, pCombat->m_players[0].m_silver - pCombat->m_players[0].m_org_silver, silver_get_boss, cdata->m_union_id, cdata->m_server_id);
        }
        else if(pCombat->m_players[0].m_silver < pCombat->m_players[0].m_org_silver)
        {
            statistics_of_silver_cost(cdata->m_id, cdata->m_ip_address, pCombat->m_players[0].m_org_silver - pCombat->m_players[0].m_silver, silver_cost_boss, cdata->m_union_id, cdata->m_server_id);
        }
#endif
        //随机声望获取
        int prestige_type = 0, prestige_value = 0;
        if (damage > 10000)
        {
            prestige_value = damage / 10000;
            if (prestige_value > 30)
                prestige_value = 30;
            prestige_type = my_random(1,4);
            int p_level = cdata->prestigeLevel(prestige_type);
            int max_value = 0, already_value = 0;
            if (p_level <= 10)
            {
                max_value = 1000;
            }
            else if(p_level <= 20)
            {
                max_value = 2000;
            }
            else if(p_level <= 30)
            {
                max_value = 5000;
            }
            else if(p_level <= 40)
            {
                max_value = 10000;
            }
            else
            {
                max_value = 20000;
            }
            already_value = m_prestige_maps[std::make_pair(acid, prestige_type)];
            if (already_value + prestige_value > max_value)
            {
                prestige_value = max_value - already_value;
                m_prestige_maps[std::make_pair(acid, prestige_type)] = max_value;
            }
            if (prestige_value > 0 && prestige_type > 0)
            {
                cdata->addPrestige(prestige_type, prestige_value, prestige_get_boss);
                Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_PRESTIGE_BEGIN+prestige_type, prestige_value, 0);
                json_spirit::Array getlist;
                json_spirit::Object o;
                tmp.toObj(o);
                getlist.push_back(o);
                pCombat->m_result_obj.push_back( Pair("get", getlist) );
            }
        }
    }
    if (pCombat->m_result == COMBAT_RESULT_ATTACK_WIN)
    {
        // 15分钟内击杀，提升boss等级
        if ((time(NULL) - _start_time) < (900 + iBossCombatDelay))
        {
            //提升所有boss等级，所有boss共享一个等级
            bossMgr::getInstance()->setLevel(_boss._level + 1);
        }
        //幸运的玩家，最后一击
        std::string msg = strBossKillMsg;
        str_replace(msg, "$W", MakeCharNameLink(cdata->m_name, cdata->m_nick.get_string()));
        str_replace(msg, "$B", _boss._name);
        //击杀奖励
        int kill_silver = 200000 + (_boss._level-iBossMinLevel) * 50000;
        Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, kill_silver, 0);
        str_replace(msg, "$S", tmp.toString(true), true);
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        close(acid);
    }
    return HC_SUCCESS;
}

int Boss::open(int last_mins)
{
    if (0 == _boss._open_state)
    {
        reset();
        _boss._open_state = 1;

        std::string msg = strBossOpenMsg;
        str_replace(msg, "$B", _boss._name);
        str_replace(msg, "$n", LEX_CAST_STR(_boss._id));
        str_replace(msg, "$S", LEX_CAST_STR(_boss._spic));
        //广播开放消息
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        //广播图标改变
        GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(strNotifyBossOpen, iBossOpenLevel);

        _start_time = time(NULL);
        _end_time = _start_time + last_mins * 60;

        json_spirit::mObject mobj;
        mobj["cmd"] = "scheduleEvent";
        mobj["event"] = "closeBoss";
        mobj["param1"] = _boss._id;

        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(_end_time-time(NULL), 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    }

    return HC_SUCCESS;
}

int Boss::close(int killer)
{
    if (_boss._open_state == 0)
    {
        return HC_SUCCESS;
    }

    splsTimerMgr::getInstance()->delTimer(_uuid);
    _uuid = boost::uuids::nil_uuid();

    GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(strNotifyBossClose, iBossOpenLevel);

    //额外奖励
    int rank = 1;
    for (std::list<bossScore>::iterator it = _topten.begin(); it != _topten.end(); ++it)
    {
        if (m_char_list[it->cid].get())
        {
            std::string msg = "";
            switch (rank)
            {
                case 1:
                    msg = strBossTop1Damage;
                    str_replace(msg, "$W", MakeCharNameLink(it->name,m_char_list[it->cid]->m_nick.get_string()), true);
                    str_replace(msg, "$B", _boss._name, true);
                    break;
                case 2:
                    msg = strBossTop2Damage;
                    str_replace(msg, "$W", MakeCharNameLink(it->name,m_char_list[it->cid]->m_nick.get_string()), true);
                    break;
                case 3:
                    msg = strBossTop3Damage;
                    str_replace(msg, "$W", MakeCharNameLink(it->name,m_char_list[it->cid]->m_nick.get_string()), true);
                    break;
            }
            if (msg != "")
            {
                GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            }
        }
        //信件和奖励发放
        std::string mailContent = strBossMail;
        str_replace(mailContent, "$B", _boss._name, true);
        str_replace(mailContent, "$D", LEX_CAST_STR(m_score_maps[it->cid]), true);
        str_replace(mailContent, "$R", LEX_CAST_STR(rank), true);
        //获取名次奖励
        std::list<Item> tmp_list;
        bossMgr::getInstance()->getAwards(rank, tmp_list);
        //增加声望获得
        str_replace(mailContent, "$G", itemlistToString(tmp_list), true);
        if (tmp_list.size())
        {
            Singleton<rewardsMgr>::Instance().updateCharRewards(it->cid,REWARDS_TYPE_BOSS,rank,tmp_list);
        }
        if (it->cid == killer)
        {
            mailContent += strBossKillMail;
            //击杀奖励
            tmp_list.clear();
            int kill_silver = 200000 + (_boss._level-iBossMinLevel) * 50000;
            Item tmp(ITEM_TYPE_CURRENCY, CURRENCY_ID_SILVER, kill_silver, 0);
            tmp_list.push_back(tmp);
            str_replace(mailContent, "$S", tmp.toString(true), true);
            if (tmp_list.size())
            {
                Singleton<rewardsMgr>::Instance().updateCharRewards(it->cid,REWARDS_TYPE_BOSS_KILL,rank,tmp_list);
            }
        }
        //发送系统邮件
        sendSystemMail(it->name, it->cid, strBossMailTitle, mailContent);
        ++rank;
    }
    //结束通知
    std::string endMsg = strBossActionEnd;
    str_replace(endMsg, "$B", LEX_CAST_STR(_boss._id));
    _channel->BroadMsg(endMsg, 0);    //调用这个接口立即发送

    _boss._open_state = 0;

    //boss战斗结束清除boss鼓舞效果
    std::map<int, int>::iterator it = m_score_maps.begin();
    for (; it != m_score_maps.end(); ++it)
    {
        if (it->second > 0)
        {
            INFO(" ############# clear inspire ####################"<<it->first<<endl);
            if (m_char_list[it->first].get())
            {
                m_char_list[it->first]->m_combat_attribute.boss_inspired(0);
                bossMgr::getInstance()->clearInspire(it->first);
            }
        }
    }
    m_attack_cd_maps.clear();
    m_damage_maps.clear();
    m_silver_maps.clear();
    m_score_maps.clear();
    m_prestige_maps.clear();
    //清理频道
    for (std::map<int, boost::shared_ptr<CharData> >::iterator it = m_char_list.begin(); it != m_char_list.end(); ++it)
    {
        _channel->Remove(it->first);
    }
    m_char_list.clear();
    m_event_log.clear();
    _topten.clear();
    INFO(" ############# close boss success ####################"<<endl);
    return HC_SUCCESS;
}

int Boss::enter(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>& ou)
{
    if (cdata.get())
    {
        if (m_char_list.find(cdata->m_id) == m_char_list.end())
        {
            m_char_list[cdata->m_id] = cdata;
            _channel->Add(ou);
        }
    }
    return HC_SUCCESS;
}

int Boss::leave(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>& ou)
{
    if (cdata.get())
    {
        m_char_list.erase(cdata->m_id);
        _channel->Remove(ou);
    }
    return HC_SUCCESS;
}
int Boss::broadBossLogEvent()
{
    json_spirit::Object robj;
    json_spirit::Array list;
    std::list<boost::shared_ptr<boss_log> >::iterator it = m_event_log.begin();
    while (it != m_event_log.end())
    {
        if ((*it).get())
        {
            json_spirit::Object o;
            std::string msg = strBossLog;
            str_replace(msg, "$A", MakeCharNameLink_other((*it)->m_name));
            str_replace(msg, "$B", _boss._name);
            str_replace(msg, "$D", LEX_CAST_STR((*it)->m_damage));
            str_replace(msg, "$S", LEX_CAST_STR((*it)->m_silver));
            o.push_back( Pair("msg", msg) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("cmd", "getBossLogEvent") );
    robj.push_back( Pair("s", 200) );
    std::string msg = json_spirit::write(robj, json_spirit::raw_utf8);
    _channel->BroadMsg(msg);
    return HC_SUCCESS;
}

int Boss::getBossLogEvent(json_spirit::Object& robj)
{
    json_spirit::Array list;
    std::list<boost::shared_ptr<boss_log> >::iterator it = m_event_log.begin();
    while (it != m_event_log.end())
    {
        if ((*it).get())
        {
            json_spirit::Object o;
            std::string msg = strBossLog;
            str_replace(msg, "$A", MakeCharNameLink_other((*it)->m_name));
            str_replace(msg, "$B", _boss._name);
            str_replace(msg, "$D", LEX_CAST_STR((*it)->m_damage));
            str_replace(msg, "$S", LEX_CAST_STR((*it)->m_silver));
            o.push_back( Pair("msg", msg) );
            list.push_back(o);
        }
        ++it;
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

bossMgr* bossMgr::m_handle = NULL;
bossMgr* bossMgr::getInstance()
{
    if (m_handle == NULL)
    {
        time_t time_start = time(NULL);
        cout<<"bossMgr::getInstance()..."<<endl;
        m_handle = new bossMgr();
        m_handle->load();
        cout<<"bossMgr::getInstance() finish,cost "<<(time(NULL)-time_start)<<" seconds"<<endl;
    }
    return m_handle;
}

bossMgr::bossMgr()
{

}

int bossMgr::load()
{
    Query q(GetDb());
    q.get_result("select id,spic,star,race,name,chat,attack,defense,magic,hp,attack_per_lev,defensee_per_lev,magic_per_lev,hp_per_lev from base_boss where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        Boss* pb = new Boss(id);
        pb->_boss._spic = q.getval();
        pb->_boss._star = q.getval();
        pb->_boss._race = q.getval();
        pb->_boss._name = q.getstr();
        pb->_boss._chat = q.getstr();
        pb->_boss._base_attack = q.getval();
        pb->_boss._base_defense = q.getval();
        pb->_boss._base_magic = q.getval();
        pb->_boss._base_hp = q.getval();
        pb->_boss._attack_per_level = q.getval();
        pb->_boss._defense_per_level = q.getval();
        pb->_boss._magic_per_level = q.getval();
        pb->_boss._hp_per_level = q.getval();

        pb->_boss._level = GeneralDataMgr::getInstance()->getInt("boss_level",iBossMinLevel);
        pb->_boss._attack = pb->_boss._base_attack;
        pb->_boss._defense = pb->_boss._base_defense;
        pb->_boss._magic = pb->_boss._base_magic;
        pb->_boss._max_hp = pb->_boss._base_hp;
        if (pb->_boss._level > iBossMinLevel)
        {
            int fac = pb->_boss._level - iBossMinLevel;
            pb->_boss._attack += fac * pb->_boss._attack_per_level;
            pb->_boss._defense += fac * pb->_boss._defense_per_level;
            pb->_boss._magic += fac * pb->_boss._magic_per_level;
            pb->_boss._max_hp += fac * pb->_boss._hp_per_level;
        }
        pb->_boss._cur_hp = pb->_boss._max_hp;
        boost::shared_ptr<Boss> spBoss(pb);
        m_boss_maps[pb->_boss._id] = spBoss;
        INFO("############### boss "<<pb->_boss._id<<",hp "<<pb->_boss._max_hp<<endl);
    }
    q.free_result();

    std::map<int, boost::shared_ptr<Boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        if (it->second.get())
        {
            Boss* pb = it->second.get();
            INFO("bossMgr::load,load from custom_shedule"<<pb->_boss._id<<endl);

            q.get_result("select day,month,hour,minute,week from custom_shedule where type='openBoss' and param1='" + LEX_CAST_STR(pb->_boss._id) + "'");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                std::string day = q.getstr();
                std::string month = q.getstr();
                std::string hour = q.getstr();
                std::string minute = q.getstr();
                std::string week = q.getstr();
                pb->_boss._open_rules.addRule(month, day, week, hour, minute);
            }
            q.free_result();
        }
        ++it;
    }

    int rank_rewards_max = 0;
    q.get_result("SELECT count(distinct(rank)) FROM base_boss_rankings_rewards where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        rank_rewards_max = q.getval();
    }
    for (int i = 1; i <= rank_rewards_max; ++i)
    {
        bossRankRewards rrr;
        rrr.rank = i;
        rrr.reward.clear();
        q.get_result("select itemType,itemId,counts,extra from base_boss_rankings_rewards where rank="+LEX_CAST_STR(i)+" order by id");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            Item item;
            item.type = q.getval();
            item.id = q.getval();
            item.nums = q.getval();
            item.extra = q.getval();
            rrr.reward.push_back(item);
        }
        q.free_result();
        if (rrr.reward.size())
        {
            m_rewards.push_back(rrr);
        }
    }
    return HC_SUCCESS;
}

//鼓舞
int bossMgr::inspire(int cid, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    int level = m_inspire_map[cid];
    if (level >= iInspireGoldMaxLevel)
    {
        return HC_ERROR_INSPIRE_MAX;
    }
    if (pc->subGold(iBossInspireGold, gold_cost_boss_inspire) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    m_inspire_map[cid] = level + 1;
    pc->m_combat_attribute.boss_inspired(m_inspire_map[cid]*iBossInspirePer);
    robj.push_back( Pair("inspire_level", m_inspire_map[cid]) );
    robj.push_back( Pair("inspire_damage", pc->m_combat_attribute.boss_inspired()) );
    robj.push_back( Pair("inspire_gold", iBossInspireGold) );
    robj.push_back( Pair("add_damage", iBossInspirePer) );
    return HC_SUCCESS;
}

//查询鼓舞等级
int bossMgr::getInspire(int cid, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        return HC_ERROR;
    }
    int level = 0;
    std::map<int,int>::iterator it = m_inspire_map.find(cid);
    if (it != m_inspire_map.end())
    {
        level = it->second;
    }
    robj.push_back( Pair("inspire_level", m_inspire_map[cid]) );
    robj.push_back( Pair("inspire_damage", pc->m_combat_attribute.boss_inspired()) );
    robj.push_back( Pair("inspire_gold", iBossInspireGold) );
    return HC_SUCCESS;
}

//清除鼓舞效果
int bossMgr::clearInspire(int cid)
{
    m_inspire_map.erase(cid);
    return 0;
}

//找找boss
boost::shared_ptr<Boss> bossMgr::getBoss(int id)
{
    std::map<int,boost::shared_ptr<Boss> >::iterator it = m_boss_maps.find(id);
    if (it != m_boss_maps.end())
    {
        return it->second;
    }
    boost::shared_ptr<Boss> boss;
    return boss;
}

int bossMgr::getBossHp(int id, json_spirit::Object& robj)
{
    robj.push_back( Pair("id", id) );
    boost::shared_ptr<Boss> spb = getBoss(id);
    if (spb.get())
    {
        if (spb->_boss._cur_hp <= spb->_boss._hide_hp)
        {
            robj.push_back( Pair("special", true) );
            robj.push_back( Pair("hide_hp", spb->_boss._hide_hp) );
            robj.push_back( Pair("hp", spb->_boss._cur_hp) );
        }
        else
        {
            robj.push_back( Pair("special", false) );
            robj.push_back( Pair("hide_hp", 0) );
            robj.push_back( Pair("hp", spb->_boss._cur_hp) );
        }
    }
    return HC_SUCCESS;
}

int bossMgr::combatResult(chessCombat* pCombat)    //boss战斗结束
{
    if (NULL == pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    Boss* pb = getBoss(pCombat->m_data_id).get();
    if (NULL == pb)
    {
        ERR();
        return HC_ERROR;
    }
    return pb->combatResult(pCombat);
}

int bossMgr::openBoss(int id, int last_mins)
{
    boost::shared_ptr<Boss> spb = getBoss(id);
    if (spb.get())
    {
        spb->open(last_mins);
    }
    return HC_SUCCESS;
}

int bossMgr::closeBoss(int id)
{
    INFO("close boss "<<id<<endl);
    boost::shared_ptr<Boss> spb = getBoss(id);
    if (spb.get())
    {
        spb->close(0);
    }
    return HC_SUCCESS;
}

int bossMgr::getAction(CharData* pc, json_spirit::Array& blist)
{
    //等级不足的不显示boss战图标
    if (!pc || (!pc->isBossOpen()))
    {
        return 0;
    }
    //获取当前时间格式匹配
    time_t timep;
    struct tm m_timenow;
    time(&timep);
    localtime_r(&timep, &m_timenow);

    //今天要开放的boss列表
    std::map<int, boost::shared_ptr<Boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        Boss* pb = it->second.get();
        if (pb)
        {
            openRule* pOpenRule = pb->_boss._open_rules.getRule(m_timenow);
            if (pb->_boss._open_state || pOpenRule)
            {
                //cout<<pb->_boss._id<<" boss state "<<pb->_boss._open_state<<",";
                json_spirit::Object obj;
                obj.push_back( Pair("type", action_boss) );
                obj.push_back( Pair("id", pb->_boss._id) );
                obj.push_back( Pair("name", pb->_boss._name) );
                obj.push_back( Pair("level", pb->_boss._level) );
                obj.push_back( Pair("spic", pb->_boss._spic) );
                if (pb->_boss._open_state)
                {
                    obj.push_back( Pair("state", 1) );
                    obj.push_back( Pair("memo", strBossOpening) );
                }
                else
                {
                    if ((pOpenRule->_open_hour > m_timenow.tm_hour
                    || (pOpenRule->_open_hour == m_timenow.tm_hour && pOpenRule->_open_min > m_timenow.tm_min)))
                    {
                        obj.push_back( Pair("state", 0) );
                        std::string memo = strBossWillOpen;
                        if (pOpenRule->_open_hour < 10)
                        {
                            str_replace(memo, "$H", "0" + LEX_CAST_STR(pOpenRule->_open_hour));
                        }
                        else
                        {
                            str_replace(memo, "$H", LEX_CAST_STR(pOpenRule->_open_hour));
                        }
                        if (pOpenRule->_open_min < 10)
                        {
                            str_replace(memo, "$M", "0" + LEX_CAST_STR(pOpenRule->_open_min));
                        }
                        else
                        {
                            str_replace(memo, "$M", LEX_CAST_STR(pOpenRule->_open_min));
                        }
                        obj.push_back( Pair("memo", memo) );
                    }
                    else
                    {
                        //结束了
                        obj.push_back( Pair("state", 2) );
                        obj.push_back( Pair("memo", strBossIsClosed) );
                    }
                }
                if (pOpenRule)
                {
                    std::string startTime = time2string(pOpenRule->_open_hour, pOpenRule->_open_min);
                    obj.push_back( Pair("startTime", startTime) );;
                    int endmin = pOpenRule->_open_min;
                    int endhour = pOpenRule->_open_hour + 2;
                    if (endhour > 23)
                    {
                        endhour -= 24;
                    }
                    std::string endTime = time2string(endhour, endmin);
                    obj.push_back( Pair("endTime", endTime) );
                }
                blist.push_back(obj);
            }
        }
        ++it;
    }
    return 1;
}

void bossMgr::setLevel(int level)    //设置所有boss等级
{
    std::map<int, boost::shared_ptr<Boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        Boss* pb = it->second.get();
        if (pb)
        {
            pb->_boss._level = level;
            pb->_boss._attack = pb->_boss._base_attack;
            pb->_boss._defense = pb->_boss._base_defense;
            pb->_boss._magic = pb->_boss._base_magic;
            pb->_boss._max_hp = pb->_boss._base_hp;
            if (pb->_boss._level > iBossMinLevel)
            {
                int fac = pb->_boss._level - iBossMinLevel;
                pb->_boss._attack += fac * pb->_boss._attack_per_level;
                pb->_boss._defense += fac * pb->_boss._defense_per_level;
                pb->_boss._magic += fac * pb->_boss._magic_per_level;
                pb->_boss._max_hp += fac * pb->_boss._hp_per_level;
            }
            GeneralDataMgr::getInstance()->setInt("boss_level",pb->_boss._level);
        }
        ++it;
    }
}

bool bossMgr::isOpen()    //boss战是否开启
{
    //今天要开放的boss列表
    std::map<int, boost::shared_ptr<Boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        Boss* pb = it->second.get();
        if (pb)
        {
            if (pb->_boss._open_state)
            {
                return true;
            }
        }
        ++it;
    }
    return false;
}

int bossMgr::getAwards(int rank, std::list<Item>& loots)
{
    for (std::vector<bossRankRewards>::iterator it = m_rewards.begin(); it != m_rewards.end(); ++it)
    {
        if (rank <= it->rank)
        {
            for (std::list<Item>::iterator it_i = it->reward.begin(); it_i != it->reward.end(); ++it_i)
            {
                loots.push_back(*it_i);
            }
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

//进出boss场景
int ProcessBossScene(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<OnlineUser> account = psession->user();
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");

    CharData* pc = cdata.get();
    if (!pc->isBossOpen())
    {
        return HC_ERROR_NEED_MORE_LEVEL;
    }
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("id", id) );
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("spic", spb->_boss._spic) );
    if (1 == type)
    {
        if (spb->_boss._open_state == 0)
        {
            return HC_ERROR_BOSS_NOT_OPEN;
        }
        spb->enter(cdata, account->m_onlineCharactor);
    }
    else
    {
        spb->leave(cdata, account->m_onlineCharactor);
    }
    return HC_SUCCESS;
}

//获取Boss信息
int ProcessGetBossInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    json_spirit::Object boss;
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    boss.push_back( Pair("id", spb->_boss._id) );
    boss.push_back( Pair("name", spb->_boss._name) );
    boss.push_back( Pair("level", spb->_boss._level) );
    boss.push_back( Pair("hp", spb->_boss._max_hp) );
    boss.push_back( Pair("spic", spb->_boss._spic) );

    robj.push_back( Pair("boss", boss) );

    json_spirit::Object info;
    int left_time = spb->_end_time - time(NULL);
    if (left_time < 0)
    {
        left_time = 1;
    }
    info.push_back( Pair("leftTime", left_time) );

    int wait_time = spb->_start_time + iBossCombatDelay - time(NULL);
    info.push_back( Pair("waitTime", wait_time > 0 ? wait_time : 0) );
    bossMgr::getInstance()->getInspire(pc->m_id, info);
    robj.push_back( Pair("info", info) );
    return HC_SUCCESS;
}

//获取Boss剩余血量
int ProcessGetBossHp(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    return bossMgr::getInstance()->getBossHp(id, robj);
}

//鼓舞
int ProcessInspire(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    return bossMgr::getInstance()->inspire(pc->m_id, robj);
}

//获取攻打Boss冷却时间
int ProcessGetCoolTime(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    Boss* pb = spb.get();
    int cd = pb->m_attack_cd_maps[pc->m_id] - time(NULL);
    if (cd < 0)
    {
        cd = 0;
    }
    robj.push_back( Pair("time", cd) );
    robj.push_back( Pair("end_cd_cost", iBossEndCDCost) );
    return HC_SUCCESS;
}

//结束攻打Boss冷却时间
int ProcessEndCoolTime(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    Boss* pb = spb.get();
    int cd = pb->m_attack_cd_maps[pc->m_id] - time(NULL);
    if (cd > 0)
    {
        if (pc->subGold(iBossEndCDCost, gold_cost_boss_cd) < 0)
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        pb->m_attack_cd_maps[pc->m_id] = 0;
    }
    return HC_SUCCESS;
}

//获取对Boss排行
int ProcessGetBossRank(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    if (!spb.get())
    {
        return HC_ERROR;
    }

    json_spirit::Array dlist;
    Boss* pb = spb.get();
    robj.push_back( Pair("damage", pb->m_damage_maps[pc->m_id]) );
    robj.push_back( Pair("silver", pb->m_silver_maps[pc->m_id]) );
    robj.push_back( Pair("score", pb->m_score_maps[pc->m_id]) );

    int rank = 0;
    for (std::list<bossScore>::iterator it = pb->_topten.begin(); it != pb->_topten.end(); ++it)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("rank", ++rank) );
        obj.push_back( Pair("name", it->name) );
        obj.push_back( Pair("score", it->score) );
        if (it->cid == pc->m_id)
        {
            obj.push_back( Pair("self", 1) );
        }
        dlist.push_back(obj);
        if (rank >= 10)
        {
            break;
        }
    }
    robj.push_back( Pair("list", dlist) );
    return HC_SUCCESS;
}

//获取公告信息
int ProcessGetBossLogEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<Boss> spb = bossMgr::getInstance()->getBoss(id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    return spb->getBossLogEvent(robj);
}

