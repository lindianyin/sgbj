
#include "boss.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include "spls_const.h"

#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"

#include "data.h"
#include "spls_timer.h"
#include "net.h"
#include "utils_lang.h"
#include <syslog.h>
#include "statistics.h"
#include "rankings.h"
#include "singleton.h"
#include "SaveDb.h"
#include "daily_task.h"
#include "rewards.h"
#include "qq_invite.h"

using namespace json_spirit;
using namespace net;

extern std::string strBossMailExtra;
extern std::string strBossMailTop4_10;

//static     boost::uuids::nil_generator gen;

const std::string strNotifyBossHp = "{\"cmd\":\"getBossHp\",\"id\":$B,\"s\":200,\"hp\":$H,\"hide_hp\":$h,\"special\":$S,\"attacker\":\"$A\"}";
const std::string strNotifyBossPerson = "{\"cmd\":\"getBossPerson\",\"id\":$B,\"s\":200,\"person\":$P}";
const std::string strBossCombatEnd = "{\"cmd\":\"bossBattleEnd\",\"id\":$B,\"s\":200}";
const std::string strNotifyBossOpen = "{\"cmd\":\"updateAction\",\"type\":2,\"s\":200,\"active\":1}";
const std::string strNotifyBossClose = "{\"cmd\":\"updateAction\",\"type\":2,\"s\":200,\"active\":0}";
const std::string strNotifyBossDying = "{\"cmd\":\"boss_dying\",\"name\":\"$B\",\"s\":200}";

extern std::string strNewBossRank1Msg;

#define INFO(x)

Database& GetDb();

extern void InsertCombat(Combat* pcombat);
extern int updateCombatAttribute(boost::shared_ptr<baseState>* state, int stateNums, combatAttribute& ct);
extern void InsertSaveDb(const std::string& sql);
extern void InsertSaveDb(const saveDbJob& job);
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);
void broadCastCorpsMsg(int corps, const std::string& msg);

//初始化boss战斗
extern Combat* createBossCombat(int cid, int tid, int& ret);
extern int createBossCombat(int cid, Combat* pCombat);

int getBossActionMemo(spls_boss* pb, json_spirit::Object& robj);

void bossRealReward(int& get);

inline double get_percent(int damage, int total_hp)
{
    double temp = (double)damage / total_hp;
    //cout<<temp<<"|";
    temp = temp * 10000.0;
    //cout<<temp<<"|";
    int iTemp = (int)temp;
    //cout<<iTemp<<endl;
    return (double)iTemp/100;
}

bool compare_damage(bossDamage& a, bossDamage& b)
{
    return a.damage > b.damage;
}

combatBoss::combatBoss(Army* army, int pos, const bossData& bd)
:General(army, pos, bd)
,_data(bd)
{
    m_damage_type = 1;
    m_damage_type2 = 5;
}

//攻击地方单个武将
//int combatBoss::Attack(iGeneral& target)
//{
//    int damage = my_random(_data._damage1, _data._damage2);
//    target.RecieveDamage(damage, *this, m_damage_type, m_damage_type2, m_base_stype, false);
//    return HC_SUCCESS;
//}

//攻击目标造成的伤害
int combatBoss::Damages(iGeneral& target)
{
    return my_random(_data._damage1, _data._damage2);
}

/*行动
int combatBoss::Action()
{
    Army* army = GetArmy().GetEnermy();
    //获得目标
    iGeneral* target = army->GetRowGeneral(POS_TO_ROW(m_pos));
    if (NULL != target)
    {
        m_action_obj.clear();
        m_result_list.clear();
        int result = ATTACK_RESULT_SUCCESS;
        if (Set_BaojiFlag(*target))
        {
            result = ATTACK_RESULT_BAOJI;
            //攻击成功加士气
            AddShiqi(2*iAttackSuccessShiqi, false);
        }
        else
        {
            //攻击成功加士气
            AddShiqi(iAttackSuccessShiqi, false);
        }
        UpdateObj();
        //命中
        int damage = Attack(*target);
        json_spirit::Object obj;
        obj.push_back( Pair("re", result) );
        obj.push_back( Pair("g", target->GetObj()) );
        obj.push_back( Pair("damage", damage) );
        m_result_list.push_back(obj);

        GetArmy().GetCombat().AppendResult(m_action_obj);

        //m_army->sendBuff();
        //m_army->GetEnermy()->sendBuff();
        return result;
    }
    else
    {
        //赢了
        return ATTACK_RESULT_SUCCESS;
    }
}*/

//受到伤害扣除生命 - 伤害减免在这里处理
int combatBoss::RecieveDamage(int damage, iGeneral& attacker, int attack_type, int injure_type, int attacker_base_type = 0, bool test = false)
{
    //int hp_old = m_hp_now;
    int org_damages = General::RecieveDamage(damage, attack_type, attacker_base_type, test);
    int damages = org_damages;// >= hp_old ? hp_old : org_damages;
    if (test)
    {
        return damages;
    }
    //死亡
    if (m_hp_now == 0)
    {
        Die(attacker);
    }
    UpdateObj();
    return damages;
}

//增加士气
int combatBoss::AddShiqi(int add, bool type)
{
    if (add > 0)
    {        
        return General::AddShiqi(add, type);
    }
    else
    {
        return 0;
    }
}

bossData::bossData()
{
     //openRules _open_rules;
    _last_rule = NULL;

     _open_state = 0;
    _spic = 0;
    _soldier_spic = 0;
    _season = 0;
    _hour = 0;
    _minute = 0;
    _last_mins = 0;

    _pufang = 0;
    _cefang = 0;
    _str = 0;
    _wisdom = 0;
    _max_hp = 0;
    _cur_hp = 0;
    _hide_hp = 0;
    _base_damage1 = 0;
    _base_damage2 = 0;
    _base_hp = 0;
    _damage1 = 0;
    _damage2 = 0;
    _level = 0;
    _damage_per_level = 0;
    _hp_per_level = 0;
    _state = 0;
    _type = 0;
    _id = 0;
    _stype = 0;

    _max_silver = 0;
    _max_prestige = 0;
    
    memset(_name, 0, 50);
    memset(_chat, 0, 200);

    _last_open_time = 0;
    //combatAttribute m_combat_attribute;    //战斗属性
}

bool compare_boss_damage_ranking(const bossDamageRanking& a, const bossDamageRanking& b)
{
    return (a.damage >= b.damage);
}

void bossDamageRankings::updateRankings(std::map<int, int>& dmap, const std::string& bossname)
{
    std::map<int, int>::iterator it = dmap.begin();
    while (it != dmap.end())
    {
        int damage = it->second;
        int cid = it->first;
        if (damageMap.size() < (size_t)iBossDamageRankingsCount
            || damage > min_damage)
        {
            if (damageMap[cid] < damage)
            {
                damageMap[cid] = damage;
                if (damage < min_damage)
                {
                    min_damage = damage;
                }
            }
        }
        ++it;
    }

    int pre_rank1 = 0;
    if (damageList.size())
    {
        pre_rank1 = damageList.begin()->cid;
    }
    
    // map -> list
    damageList.clear();
    for (std::map<int, int>::iterator itx = damageMap.begin(); itx != damageMap.end(); ++itx)
    {
        bossDamageRanking dr;
        dr.cid = itx->first;
        dr.damage = itx->second;
        damageList.push_back(dr);
        //cout<<"push "<<dr.cid<<","<<dr.damage<<endl;
    }

    //sort list
    damageList.sort(compare_boss_damage_ranking);

    //去掉多餘的
    while (damageList.size() > (size_t)iBossDamageRankingsCount)
    {
        damageList.pop_back();
    }

    //最低傷害
    min_damage = 0;
    if (damageList.size())
    {
        min_damage = damageList.rbegin()->damage;

        if ("" != bossname)
        {
            //第一名是否易主
            int new_rank1 = damageList.begin()->cid;
            if (new_rank1 != pre_rank1)
            {
                //廣播
                CharData* pc = GeneralDataMgr::getInstance()->GetCharData(new_rank1).get();
                if (pc)
                {
                    //$C对$B造成$D点伤害，勇夺神威榜首！
                    std::string msg = strNewBossRank1Msg;
                    str_replace(msg, "$C", MakeCharNameLink(pc->m_name), true);
                    str_replace(msg, "$B", bossname, true);
                    str_replace(msg, "$D", LEX_CAST_STR(damageList.begin()->damage), true);
                    GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                }
            }
        }
    }

    damageMap.clear();

    for (int i = 0; i < iBossDamageRankingsPage; ++i)
    {
        strRangkingsPages[i] = "";
        aRankingsPages[i].clear();
    }

    saveDbJob job;
    job.sqls.push_back("TRUNCATE TABLE char_boss_damage_rankings");

    //生成每頁的array
    int count = 0;
    int maxPage = 0;

    memset(rankings, 0, sizeof(int)*iBossDamageRankingsCount);
    std::list<bossDamageRanking>::iterator itr = damageList.begin();
    while (itr != damageList.end())
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(itr->cid).get();
        if (pc)
        {
            if (count >= 0 && count < iBossDamageRankingsCount)
            {
                rankings[count] = pc->m_id;
            }
            int pageIdx = count /iBossDamageRankingsPerPage;
            maxPage = pageIdx + 1;
            json_spirit::Object obj;
            obj.push_back( Pair("rank", count + 1) );
            obj.push_back( Pair("camp", pc->m_camp) );
            obj.push_back( Pair("name", pc->m_name) );
            json_spirit::Array a(pc->m_nick.m_nick_list.begin(), pc->m_nick.m_nick_list.end());
            obj.push_back( Pair("nick", a) );
            obj.push_back( Pair("level", pc->m_level) );
            obj.push_back( Pair("damage", itr->damage) );
            aRankingsPages[pageIdx].push_back(obj);
            ++count;
            //cout<<"char_boss_damage_rankings,cid "<<itr->cid<<","<<itr->damage<<endl;
            job.sqls.push_back("insert into `char_boss_damage_rankings` set cid=" + LEX_CAST_STR(itr->cid) + ",damage=" + LEX_CAST_STR(itr->damage));
            //重新生成map
            damageMap[itr->cid] = itr->damage;
        }
        ++itr;
    }

    InsertSaveDb(job);

    //生成每頁的string
    for (int i = 0; i < iBossDamageRankingsPage; ++i)
    {
        if (aRankingsPages[i].size())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("cmd", "getBossRankList") );
            obj.push_back( Pair("s", 200) );
            json_spirit::Object pageobj;
            pageobj.push_back( Pair("maxPage", maxPage) );
            pageobj.push_back( Pair("page", i + 1) );
            pageobj.push_back( Pair("pageNums", iBossDamageRankingsPerPage) );

            obj.push_back( Pair("list", aRankingsPages[i]) );
            obj.push_back( Pair("page", pageobj) );

            strRangkingsPages[i] = json_spirit::write(obj);
        }
    }
}

//更新排行榜活动中的cid字段
void bossDamageRankings::updateRankingsEvent(rankings_event* pE)
{
    //cout<<"splsRankings::updateRankingsEvent(),type="<<pE->type<<endl;
    for (std::list<rankings_event_award>::iterator it = pE->rankings_list.begin(); it != pE->rankings_list.end(); ++it)
    {
        //cout<<"rank:"<<it->rank<<endl;
        if (it->rank > 0 && it->rank <= iBossDamageRankingsCount)
        {
            it->cid = rankings[it->rank-1];
            //cout<<"\tcid->"<<it->cid<<endl;
        }
    }
}

std::string bossDamageRankings::getRankings(int page)
{
    if (page <= 0 || page > iBossDamageRankingsPage)
    {
        page = 1;
    }
    if (strRangkingsPages[page-1] != "")
    {
        return strRangkingsPages[page-1];
    }
    else
    {
        return "{\"cmd\":\"getBossRankList\",\"s\":201}";
    }
}

spls_boss::spls_boss(int id)
:m_stand_in_mob(baby_boss_start+id)
{
    //boss的攻击无法躲闪/格挡/识破, 可以被暴击
    _boss.m_combat_attribute.special_resist(special_attack_dodge, 100000);
    _boss.m_combat_attribute.special_resist(special_attack_parry, 100000);
    _boss.m_combat_attribute.special_resist(special_attack_shipo, 100000);
    _boss.m_combat_attribute.special_resist(special_attack_xixue, 100000);
    _boss.m_combat_attribute.special_resist(special_attack_chaos, 100000);
    _boss.m_combat_attribute.special_resist(special_attack_podan, 100000);
    _boss.m_combat_attribute.special_resist(special_attack_weihe, 100000);
    
    _boss.m_combat_attribute.add_resist_level(special_attack_dodge, 500000);
    _boss.m_combat_attribute.add_resist_level(special_attack_parry, 500000);
    _boss.m_combat_attribute.add_resist_level(special_attack_shipo, 500000);
    _boss.m_combat_attribute.add_resist_level(special_attack_xixue, 500000);
    _boss.m_combat_attribute.add_resist_level(special_attack_chaos, 500000);
    _boss.m_combat_attribute.add_resist_level(special_attack_podan, 500000);
    _boss.m_combat_attribute.add_resist_level(special_attack_weihe, 500000);

    _corps = 0;
    //memset(&_boss, 0, sizeof(bossData));
    _boss._id = id;
    _boss._state = 0;
    _boss._stype = 6;
    _pCombat = NULL;
    _start_time = 0;
    _end_time = 0;
    _attacker = 0;
    _type = 1;
    _uuid = boost::uuids::nil_uuid();

    _boss_hp_msg = strNotifyBossHp;
    str_replace(_boss_hp_msg, "$B", LEX_CAST_STR(_boss._id));

    _boss_person_msg = strNotifyBossPerson;
    str_replace(_boss_person_msg, "$B", LEX_CAST_STR(_boss._id));

    _boss_will_open = strBossWillOpen;
    _boss_open_time = "00:00";

    _channel.reset(new ChatChannel("boss", id, "{\"cmd\":\"chat\",\"ctype\":10,\"s\":200,"));
    _channel->start();
}

//是否能攻击
int spls_boss::canAttack(int cid, int& cool)
{
    if (_boss._open_state == 0)
    {
        return HC_ERROR_BOSS_EVENT_END;
    }
#ifdef TEST_SERVER
    if (time(NULL) - _start_time < 10)
#else
    if (time(NULL) - _start_time < iBossCombatDelay)
#endif
    {
        return HC_ERROR_BOSS_WAITING;
    }
    if (m_char_enter.find(cid) == m_char_enter.end())
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
    cd = m_attack_min_cd_maps[cid] - time(NULL);
    if (cd > 0)
    {
        cool = cd;
        return HC_ERROR_IN_COOLTIME;
    }
    return HC_SUCCESS;
}

void spls_boss::_tryAttack(int cid, int type)
{
    _attacker = cid;
    _type = type;
    int ret = HC_SUCCESS;
    bool bFirst = false;
    if (_pCombat)
    {
        if (_pCombat->m_attacker)
        {
            delete _pCombat->m_attacker->_army_data;
            delete _pCombat->m_attacker;
            _pCombat->m_attacker = NULL;
        }
        ret = createBossCombat(cid, _pCombat);
    }
    else
    {
        _pCombat = createBossCombat(cid, _boss._id, ret);
        bFirst = true;
    }
    if (HC_SUCCESS != ret || !_pCombat)
    {
        INFO("############### start a attack boss combat "<<cid<<endl);
        if (bFirst)
        {
            if (_pCombat)
            {
                delete _pCombat;
                _pCombat = NULL;
            }
        }
        if (m_attack_list.size())
        {
            boss_attacker atk = m_attack_list.front();
            _attacker = atk.cid;
            _type = atk.type;
            m_attack_list.pop_front();
            _tryAttack(_attacker, _type);
        }
        else
        {
            _attacker = 0;
            _type = 0;
        }
    }
    else
    {
        INFO("############### start a attack boss combat "<<cid<<endl);

        if (2 != _type)
        {
            //立即返回战斗双方的信息
            boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(_pCombat->m_attacker->Name());
            if (pchar.get())
            {
                json_spirit::Object robj;
                robj.push_back( Pair ("cmd", "attack") );
                robj.push_back( Pair ("getBattleList", _pCombat->getCombatInfo()));
                robj.push_back( Pair("s", 200) );
                std::string final_result = json_spirit::write(robj, json_spirit::raw_utf8);
                pchar->Send(final_result);
            }
        }
        InsertCombat(_pCombat);
    }    
}

int spls_boss::tryAttack(int cid, int type, json_spirit::Object& robj)
{
    //冷却时间20+30秒
    m_attack_min_cd_maps[cid] = time(NULL) + iBossMinCombatCD;
    m_attack_cd_maps[cid] = m_attack_min_cd_maps[cid] + iBossCombatCD;

    //如果当前有人在攻击，加入攻击队列
    if (_attacker > 0)
    {
        boss_attacker atk;
        atk.cid = cid;
        atk.type = type;
        m_attack_list.push_back(atk);
        return HC_SUCCESS_NO_RET;
    }
    _tryAttack(cid, type);
    return HC_SUCCESS_NO_RET;
}

int spls_boss::reset()
{
    _boss._cur_hp = _boss._max_hp;
    _boss._hide_hp = _boss._max_hp / 20;
    _start_time = 0;
    _end_time = 0;
    if (_pCombat)
    {
        if (_pCombat->m_attacker)
        {
            delete (_pCombat->m_attacker->_army_data);
            delete _pCombat->m_attacker;
        }
        if (_pCombat->m_defender)
        {
            delete (_pCombat->m_defender->_army_data);
            delete _pCombat->m_defender;
        }
        delete _pCombat;
        _pCombat = NULL;
    }
    return HC_SUCCESS;
}

int spls_boss::levelup()
{
    ++_boss._level;
    _boss._damage1 += _boss._damage_per_level;
    _boss._damage2 += _boss._damage_per_level;
    _boss._max_hp += _boss._hp_per_level;
    
    _boss._max_silver = 3000 + (_boss._level - 30) * 100;
    _boss._max_prestige = 30;
    
    InsertSaveDb("update char_corps_boss set level=" + LEX_CAST_STR(_boss._level) + " where corps=" + LEX_CAST_STR(_corps));
    return HC_SUCCESS;
}

int spls_boss::combatResult(Combat* pCombat)    //boss战斗结束
{
    //BOSS奄奄一息
    if (_boss._cur_hp > _boss._hide_hp && pCombat->m_defender->TotalHp() <= _boss._hide_hp)
    {
        if (_corps == 0)
        {
            std::string msg = strNotifyBossDying;
            str_replace(msg, "$B", _boss._name);
            GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(msg, iBossOpenStronghold/2+1);
            
            msg = strBossDyingMsg;
            str_replace(msg, "$B", _boss._name);
            str_replace(msg, "$n", LEX_CAST_STR(_boss._id));
            str_replace(msg, "$S", LEX_CAST_STR(_boss._spic));
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        else
        {
            std::string msg = strNotifyBossDying;
            str_replace(msg, "$B", _boss._name);
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(_corps);
            if (gch)
            {
                gch->BroadMsg(msg);
            }
            
            msg = strJtBossDyingMsg;
            str_replace(msg, "$B", _boss._name);
            str_replace(msg, "$n", LEX_CAST_STR(_boss._id));
            str_replace(msg, "$S", LEX_CAST_STR(_boss._spic));
            broadCastCorpsMsg(_corps, msg);
        }
    }
    int damage = _boss._cur_hp - pCombat->m_defender->TotalHp();
    _boss._cur_hp = pCombat->m_defender->TotalHp();

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
        str_replace(hpmsg, "$A", pCombat->m_attacker->Name());
        _channel->BroadMsg(hpmsg);
    }

    int acid = pCombat->m_attacker->getCharId();
    m_damage_maps[acid] += damage;

    //角色最高傷害
    if (m_max_damage_maps[acid] < damage)
    {
        m_max_damage_maps[acid] = damage;
    }
    
    int rank = 0;
    bool find = false;
    bool bTop10_change = false;
    for (std::list<bossDamage>::iterator it = _topten_damages.begin(); it != _topten_damages.end(); ++it)
    {
        ++rank;
        if ((*it).cid == acid)
        {
            (*it).damage = m_damage_maps[acid];
            (*it).percent = get_percent((*it).damage , _boss._max_hp);
            INFO(" damage "<<damage<<",total damage "<<it->damage<<",percent:"<<it->percent<<endl);
            find = true;
            break;
        }
    }
    if (!find)
    {
        bossDamage bossd;
        bossd.cid = acid;
        bossd.damage = m_damage_maps[bossd.cid];
        bossd.percent = get_percent(bossd.damage, _boss._max_hp);
        bossd.name = pCombat->m_attacker->Name();
        INFO(" damage "<<damage<<",total damage "<<bossd.damage<<",percent:"<<bossd.percent<<endl);
        _topten_damages.push_back(bossd);
    }

    _topten_damages.sort(compare_damage);

    if (rank <= 10)
    {
        bTop10_change = true;
    }
    else
    {
        int rank = 1;
        for (std::list<bossDamage>::iterator it = _topten_damages.begin(); it != _topten_damages.end() && rank <= 5; ++it, ++rank)
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
        robj.push_back( Pair("cmd", "getBossDamage") );
        robj.push_back( Pair("id", _boss._id) );
        robj.push_back( Pair("s", 200) );
        json_spirit::Array dlist;
        int rank = 0;
        for (std::list<bossDamage>::iterator it = _topten_damages.begin(); it != _topten_damages.end(); ++it)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("rank", ++rank) );
            obj.push_back( Pair("name", it->name) );
            obj.push_back( Pair("damage", it->damage) );
            obj.push_back( Pair("percent", it->percent) );            
            dlist.push_back(obj);
            if (rank >= 10)
            {
                break;
            }
        }
        robj.push_back( Pair("list", dlist) );
        INFO("#################### boss combat broad top 10 damager #########################"<<endl);
        _channel->BroadMsg(write(robj, json_spirit::raw_utf8));
    }

    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(acid);
    //刷新自己伤害信息
    if (cdata.get())
    {
        json_spirit::Object robj;
        robj.push_back( Pair("cmd", "getBossDamage") );
        robj.push_back( Pair("id", _boss._id) );
        robj.push_back( Pair("s", 200) );
        robj.push_back( Pair("damage", m_damage_maps[acid]) );
        robj.push_back( Pair("percent", get_percent(m_damage_maps[acid], _boss._max_hp)) );
        cdata->sendObj(robj);
    }

    int silver1 = 0;
    if (_corps == 0)
    {
        silver1 = pCombat->m_state == attacker_win ? (_boss._level - 20) * 100000 : 0;
    }
    else
    {
        //击杀奖励初始为5000玉石。每升一级加1000,初始等级30
        silver1 = pCombat->m_state == attacker_win ? (_boss._level - 25) * 1000 : 0;        
    }
    int silver = damage / 10;
    if (silver > cdata->m_level * 120)
    {
        silver = cdata->m_level * 120;
    }
    //神兽实际收益
    bossRealReward(silver);
    
    //if (silver > _boss._max_silver)
    //{
    //    silver = _boss._max_silver;
    //}
    //silver += silver1;
    int prestige = damage / 10000;
    if (prestige == 0)
        prestige = 1;
    if (prestige > _boss._max_prestige)
    {
        prestige = _boss._max_prestige;
    }
    //神兽实际收益
    bossRealReward(prestige);

    if (cdata.get())
    {
        int bossMaxPrestige = iBossCombatExpLimit;
        //神兽实际收益
        bossRealReward(bossMaxPrestige);
        if ((m_prestige_gets[cdata->m_id] + prestige) > bossMaxPrestige)
        {
            prestige = bossMaxPrestige - m_prestige_gets[cdata->m_id];
        }

        if (silver > 0)
        {
            Item item_silver;
            item_silver.type = item_type_silver;
            item_silver.nums = silver;
            pCombat->m_getItems.push_back(item_silver);
        }
        if (prestige > 0)
        {
            Item item_p;
            item_p.type = item_type_prestige;
            item_p.nums = prestige;
            
            pCombat->m_getItems.push_back(item_p);
            //corpsMgr::getInstance()->addEvent(cdata.get(), corps_event_add_exp, prestige, 0);
        }
        giveLoots(cdata, pCombat, true, give_boss_loot);

        m_prestige_gets[cdata->m_id] += prestige;
        m_silver_gets[cdata->m_id] += silver;
        INFO(" @@@@@@@@@@@@@@@@@@@@ "<<cdata->m_id<<" @total prestige "<<m_prestige_gets[cdata->m_id]<<" @total silver "<<m_silver_gets[cdata->m_id]<<endl);
    }

    INFO(" ############# boss combat result send report ####################"<<endl);
    std::string memo = strBossDamageReport;
    str_replace(memo, "$W", pCombat->m_attacker->Name());
    str_replace(memo, "$B", pCombat->m_defender->Name());
    str_replace(memo, "$D", LEX_CAST_STR(damage));
    INFO(" ############# boss combat result send report 2 ####################"<<endl);

    if (pCombat->m_state == attacker_win)
    {
        pCombat->m_result_obj.push_back( Pair("memo", strBossKillReport + memo) );
    }
    else
    {
        pCombat->m_result_obj.push_back( Pair("memo", memo) );
    }
    //战斗结果    
    boost::shared_ptr<OnlineCharactor> pchar = GeneralDataMgr::getInstance()->GetOnlineCharactor(pCombat->m_attacker->Name());
    if (pchar.get())
    {
        if (_type == 2)
        {
            json_spirit::Object robj;
            robj.push_back( Pair ("cmd", "attackBoss") );
            robj.push_back( Pair("damage", damage) );
            robj.push_back( Pair("silver", silver) );
            robj.push_back( Pair("prestige", prestige) );
            robj.push_back( Pair("s", 200) );
            pchar->Send(json_spirit::write(robj));
        }
        else
        {
            try
            {
                pCombat->AppendResult(pCombat->m_result_obj);
                json_spirit::Object final_obj;
                final_obj.push_back( Pair("cmd", "battle") );
                final_obj.push_back( Pair("s", 200) );
                final_obj.push_back( Pair("cmdlist", pCombat->m_result_array) );
                std::string final_result = json_spirit::write(final_obj, json_spirit::raw_utf8);

                pchar->Send(final_result);
            }
            catch (std::exception& e)
            {
                syslog(LOG_ERR, "--- boss report fail ---");
                syslog(LOG_ERR, "--- Exception: %s ---", e.what());
            }
        }
    }
    INFO(" ############# boss combat result send report 3 ####################"<<endl);

    if (pCombat->m_state == attacker_win)
    {
        // 15分钟内击杀，提升boss等级
        if ((time(NULL) - _start_time) < (900 + iBossCombatDelay))
        {
#if 1
            if (_corps == 0)
            {
                //提升所有boss等级，所有boss共享一个等级
                bossMgr::getInstance()->setLevel(_boss._level + 1);
            }
            else
            {
                levelup();
            }
#else
            ++_boss._level;
            if (_boss._level > 30)
            {
                int fac = _boss._level - 30;
                _boss._damage1 += fac * _boss._damage_per_level;
                _boss._damage2 += fac * _boss._damage_per_level;
                _boss._max_hp += fac * _boss._hp_per_level;
            }
            InsertSaveDb("update custom_boss set level=" + LEX_CAST_STR(_boss._level)
                    + ",damage1=" + LEX_CAST_STR(_boss._damage1)
                    + ",damage2=" + LEX_CAST_STR(_boss._damage2)
                    + ",max_hp=" + LEX_CAST_STR(_boss._max_hp)
                    + " where id=" + LEX_CAST_STR(_boss._id)
                );
#endif
        }

        if (_corps == 0)
        {
            //幸运的玩家，最后一击
            std::string msg = strBossKillMsg;
            str_replace(msg, "$W", MakeCharNameLink(pCombat->m_attacker->Name()));
            str_replace(msg, "$B", pCombat->m_defender->Name());
            str_replace(msg, "$S", LEX_CAST_STR(silver1));

            _report = "Level:" + LEX_CAST_STR(_boss._level);
            _report += "Number of participants:" + LEX_CAST_STR(m_damage_maps.size()) + "\n";
            _report += "Kill by:" + pCombat->m_attacker->Name() + "(" + LEX_CAST_STR(pCombat->m_attacker->getCharId()) + ")\n";

            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
        }
        else
        {
            //幸运的玩家，最后一击
            std::string msg = strJtBossKillMsg;
            str_replace(msg, "$W", MakeCharNameLink(pCombat->m_attacker->Name()));
            str_replace(msg, "$B", pCombat->m_defender->Name());
            str_replace(msg, "$S", LEX_CAST_STR(silver1));

            broadCastCorpsMsg(_corps, msg);
        }

        close(1, acid, silver1);
    }
    else
    {
        if (_boss._open_state)
        {
            if (m_attack_list.size())
            {
                boss_attacker atk = m_attack_list.front();
                _attacker = atk.cid;
                _type = atk.type;
                m_attack_list.pop_front();
                _tryAttack(_attacker, _type);
            }
            else
            {
                _attacker = 0;
            }
        }
        else
        {
            _attacker = 0;
            _type = 0;
            m_attack_list.clear();
        }
    }

    return HC_SUCCESS;
}

int spls_boss::open(int last_mins)
{
    if (0 == _boss._open_state)
    {
        reset();
        _boss._open_state = 1;

        if (_corps == 0)
        {
            std::string msg = strBossOpenMsg;
            str_replace(msg, "$B", _boss._name);
            str_replace(msg, "$n", LEX_CAST_STR(_boss._id));
            str_replace(msg, "$S", LEX_CAST_STR(_boss._spic));
            //广播开放消息
            GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            msg = strNotifyBossOpen;
            //str_replace(msg, "$P", LEX_CAST_STR(_boss._spic));
            //广播图标改变
            GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(msg, iBossOpenStronghold/2+1);

            _start_time = time(NULL);
            _end_time = _start_time + last_mins * 60;

            //替身娃娃扣金币
            m_stand_in_mob.processGold();
        }
        else
        {
            std::string msg = strBossOpenMsg;
            str_replace(msg, "$B", _boss._name);
            str_replace(msg, "$n", LEX_CAST_STR(_boss._id));
            str_replace(msg, "$S", LEX_CAST_STR(_boss._spic));
            //广播开放消息
            broadCastCorpsMsg(_corps, msg);

            //广播图标改变
            boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(_corps);
            if (gch)
            {
                gch->BroadMsg("{\"cmd\":\"updateAction\",\"type\":14,\"s\":200,\"active\":1}");
            }

            _start_time = time(NULL);
            _end_time = _start_time + last_mins * 60;

            _corps_state = 1;
            InsertSaveDb("update char_corps_boss set last_open=" + LEX_CAST_STR(_start_time) + " where corps=" + LEX_CAST_STR(_corps));
        }

        json_spirit::mObject mobj;
        mobj["cmd"] = "scheduleEvent";
        mobj["event"] = "closeBoss";
        mobj["param1"] = _boss._id;
        mobj["param2"] = _corps;

        _report = "boss :" + std::string(_boss._name) + ", level:" + LEX_CAST_STR(_boss._level) + "\n";
        boost::shared_ptr<splsTimer> tmsg;
        tmsg.reset(new splsTimer(_end_time-time(NULL), 1,mobj,1));
        _uuid = splsTimerMgr::getInstance()->addTimer(tmsg);        
    }

    return HC_SUCCESS;
}

int spls_boss::close(int type, int killer, int silver1)
{
    if (_boss._open_state == 0)
    {
        return HC_SUCCESS;
    }
    if (1 == type)
    {
        splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
        //GeneralDataMgr::getInstance()->broadCastSysMsg("boss event is closed!", 1);
    }
    else
    {
        splsTimerMgr::getInstance()->delTimer(_uuid);
        _uuid = boost::uuids::nil_uuid();
        _report = "Level:" + LEX_CAST_STR(_boss._level);
        _report += "Number of participants:" + LEX_CAST_STR(m_damage_maps.size()) + "\n";
    }

    if (_corps == 0)
    {
        std::string msg = strNotifyBossClose;
        //str_replace(msg, "$P", LEX_CAST_STR(_boss._spic));
        GeneralDataMgr::getInstance()->GetWorldChannel()->BroadMsg(msg, iBossOpenStronghold/2+1);

        //额外奖励
        int rank = 1;
        for (std::list<bossDamage>::iterator it = _topten_damages.begin(); it != _topten_damages.end(); ++it)
        {
            std::string mailContent = strBossMail;
            msg = "";
            int prestige = 0, baoshi = 0, yushi = 0;
            _report += LEX_CAST_STR(rank) + "\t" + it->name + "(" + LEX_CAST_STR(it->cid) + ")" + "\t\t" + LEX_CAST_STR(it->damage) + "\n";
            switch (rank)
            {
                case 1:
                    msg = strBossTop1Damage;
                    str_replace(msg, "$W", MakeCharNameLink(it->name), true);
                    str_replace(msg, "$B", _boss._name, true);
                    prestige = 600;
                    yushi = 10000;
                    break;
                case 2:
                    msg = strBossTop2Damage;
                    str_replace(msg, "$W", MakeCharNameLink(it->name), true);
                    prestige = 500;
                    yushi = 7000;
                    break;
                case 3:
                    msg = strBossTop3Damage;
                    str_replace(msg, "$W", MakeCharNameLink(it->name), true);
                    prestige = 400;
                    yushi = 5000;
                    break;
                case 4:
                    prestige = 300;
                    yushi = 3000;
                    break;
                case 5:
                case 6:        //第6名
                case 7:        //第7名
                case 8:        //第8名
                case 9:        //第9名
                case 10:    //第10名
                    prestige = 300;
                    yushi = 1800;
                    break;
            }
            if (msg != "")
            {
                GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            }
            //奖励暂存
            std::list<Item> tmp_list;
            if (prestige > 0)
            {
                //神兽实际收益
                bossRealReward(prestige);
                Item item_p;
                item_p.type = item_type_prestige;
                item_p.nums = prestige;
                
                tmp_list.push_back(item_p);
            }
            if (baoshi > 0)
            {
                Item item_p;
                item_p.type = item_type_baoshi;
                item_p.id = baoshi;
                item_p.nums = 1;
                tmp_list.push_back(item_p);
            }
            if (yushi > 0)
            {
                //神兽实际收益
                bossRealReward(yushi);
                Item item_p;
                item_p.type = item_type_treasure;
                item_p.id = treasure_type_yushi;
                item_p.nums = yushi;
                tmp_list.push_back(item_p);
            }
            if (rank <= 10)
            {
                Singleton<char_rewards_mgr>::Instance().updateCharRewards(it->cid,rewards_type_boss,rank,tmp_list);
            }
            
            str_replace(mailContent, "$B", _boss._name, true);
            str_replace(mailContent, "$D", LEX_CAST_STR(m_damage_maps[it->cid]), true);
            str_replace(mailContent, "$R", LEX_CAST_STR(rank), true);
            str_replace(mailContent, "$S", LEX_CAST_STR(m_silver_gets[it->cid]), true);
            str_replace(mailContent, "$P", LEX_CAST_STR(m_prestige_gets[it->cid]), true);
            INFO(rank<<" ######################### "<<it->cid<<" @total prestige "<<m_prestige_gets[it->cid]<<" @total silver "<<m_silver_gets[it->cid]<<endl);
            if (it->cid == killer)
            {
                mailContent += strBossKillMail;
                str_replace(mailContent, "$S", LEX_CAST_STR(silver1), true);
                tmp_list.clear();
                if (silver1 > 0)
                {
                    Item item_p;
                    item_p.type = item_type_silver;
                    item_p.nums = silver1;
                    tmp_list.push_back(item_p);
                }
                Singleton<char_rewards_mgr>::Instance().updateCharRewards(it->cid,rewards_type_boss_kill,0,tmp_list);
                #if 0
                boost::shared_ptr<CharData> c = GeneralDataMgr::getInstance()->GetCharData(it->cid);
                if (c.get())
                {
                    add_statistics_of_silver_get(c->m_id, c->m_ip_address,silver1,silver_get_boss_last, c->m_union_id, c->m_server_id);
                }
                #endif
            }
            //前10額外獎勵信息
            if (rank <= 3)
            {
                mailContent += strBossMailTop1_3;
                str_replace(mailContent, "$P", LEX_CAST_STR(prestige), true);
                if (baoshi)
                {
                    baseNewBaoshi* tr = Singleton<newBaoshiMgr>::Instance().getBaoshi(baoshi);
                    if (tr)
                    {
                        str_replace(mailContent, "$G", tr->name, true);
                    }
                }
                else if (yushi)
                {
                    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(treasure_type_yushi);
                    if (bt.get())
                    {
                        str_replace(mailContent, "$G", LEX_CAST_STR(yushi) + bt->name, true);
                    }
                }
            }
            else if (rank <= 10)
            {
                mailContent += strBossMailTop4_10;
                str_replace(mailContent, "$P", LEX_CAST_STR(prestige), true);
            }
            //伤害超过1%的，额外奖励20000银币
            if (it->percent > 1.0)
            {
                int extra_silver = 20000;
                //神兽实际收益
                bossRealReward(extra_silver);
                mailContent += strBossMailExtra;
                str_replace(mailContent, "$S", LEX_CAST_STR(extra_silver), true);

                boost::shared_ptr<CharData> c = GeneralDataMgr::getInstance()->GetCharData(it->cid);
                if (c.get())
                {
                    c->addSilver(extra_silver);
                    add_statistics_of_silver_get(c->m_id, c->m_ip_address,extra_silver,silver_get_boss, c->m_union_id, c->m_server_id);
                }
            }
            //发送系统邮件
            sendSystemMail(it->name, it->cid, strBossMailTitle, mailContent);
#ifdef QQ_PLAT
            //BOSS分享
            Singleton<inviteMgr>::Instance().update_event(it->cid, SHARE_EVENT_FIRST_BOSS, 0);
#endif
            ++rank;
        }

        //保存本次boss结果
        InsertSaveDb("insert into char_mails (input,unread,archive,type,`from`,title,content,cid) values (now(),'1','0','1',0,'boss result','"
                    + GetDb().safestr(_report)    + "','0')");
    }
    else
    {
        //广播图标改变
        boost::shared_ptr<ChatChannel> gch = GeneralDataMgr::getInstance()->GetGuildChannel(_corps);
        if (gch)
        {
            gch->BroadMsg("{\"cmd\":\"updateAction\",\"type\":14,\"s\":200,\"active\":0}");
        }

        if (silver1 > 0)
        {
            std::list<Item> tmp_list;
            Item item_p;
            item_p.type = item_type_treasure;
            item_p.id = treasure_type_yushi;
            item_p.nums = silver1;
            tmp_list.push_back(item_p);
            Singleton<char_rewards_mgr>::Instance().updateCharRewards(killer,rewards_type_jt_boss,0,tmp_list);
        }

        _corps_state = 2;

        int rank = 1;
        for (std::list<bossDamage>::iterator it = _topten_damages.begin(); it != _topten_damages.end(); ++it)
        {
            std::string mailContent = strBossMail;
            str_replace(mailContent, "$B", _boss._name, true);
            str_replace(mailContent, "$D", LEX_CAST_STR(m_damage_maps[it->cid]), true);
            str_replace(mailContent, "$R", LEX_CAST_STR(rank), true);
            str_replace(mailContent, "$S", LEX_CAST_STR(m_silver_gets[it->cid]), true);
            str_replace(mailContent, "$P", LEX_CAST_STR(m_prestige_gets[it->cid]), true);
            if (it->cid == killer)
            {
                mailContent += strJtBossKillMail;
                str_replace(mailContent, "$S", LEX_CAST_STR(silver1), true);
            }
            //发送系统邮件
            sendSystemMail(it->name, it->cid, strBossMailTitle, mailContent);
            ++rank;
        }
    }
    //结束通知
    std::string endMsg = strBossCombatEnd;
    str_replace(endMsg, "$B", LEX_CAST_STR(_boss._id));
    _channel->BroadMsg(endMsg, 0);    //调用这个接口立即发送

    _boss._open_state = 0;

    std::map<int, int>::iterator it = m_damage_maps.begin();

    //boss战斗结束清除boss鼓舞效果
    for (; it != m_damage_maps.end(); ++it)
    {
        if (it->second > 0)
        {
            INFO(" ############# clear inspire ####################"<<it->first<<endl);
            if (m_char_list[it->first].get())
            {
                m_char_list[it->first]->m_combat_attribute.boss_inspired(0);
                bossMgr::getInstance()->clearInspire(it->first);
                //act统计
                act_to_tencent(m_char_list[it->first].get(),act_new_boss,_boss._id);
            }
        }
    }
    _attacker = 0;
    _type = 1;

    int top_damage_count = 0, top_10_presige = 0, top_10_attack = 0;
    for (std::list<bossDamage>::iterator it = _topten_damages.begin(); it != _topten_damages.end(); ++it)
    {
        top_10_presige += m_prestige_gets[it->cid];
        boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(it->cid);
        if (cdata.get())
        {
            top_10_attack += cdata->getAttack();
        }
        ++top_damage_count;
        if (top_damage_count >= 10)
        {
            break;
        }
    }
    int prestige_fac = 0, attack_fac = 0;
    if (top_damage_count > 0)
    {
        prestige_fac = top_10_presige/top_damage_count;
        attack_fac = top_10_attack/top_damage_count;
    }

    if (_corps == 0)
    {
        //替身娃娃给奖励
        m_stand_in_mob.processReward(prestige_fac, attack_fac);
    }

    m_attack_list.clear();        //攻击队列

    m_attack_cd_maps.clear();    //玩家的攻击冷却时间
    m_attack_min_cd_maps.clear();
    m_attack_end_cd_times.clear();
    
    m_damage_maps.clear();        //玩家的输出伤害
    m_char_enter.clear();        //副本中的角色进入时间
    
    m_silver_gets.clear();        //清空累计获得银币
    m_prestige_gets.clear();    //清空累计获得声望

    for (std::map<int, boost::shared_ptr<CharData> >::iterator it = m_char_list.begin(); it != m_char_list.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->m_copy_id = 0;
        }
        _channel->Remove(it->first);
    }
    m_char_list.clear();        //副本中的角色

    _topten_damages.clear();

    if (_corps == 0)
    {
        //更新傷害榜
        bossMgr::getInstance()->updateRankings(m_max_damage_maps, _boss._name);
    }

    m_max_damage_maps.clear();
    
    INFO(" ############# close boss success ####################"<<endl);
    return HC_SUCCESS;
}

int spls_boss::enter(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>& ou)
{
    if (cdata.get())
    {
        if (m_char_list.find(cdata->m_id) == m_char_list.end())
        {
            m_char_list[cdata->m_id] = cdata;
            m_char_enter[cdata->m_id] = time(NULL);
            cdata->m_copy_id = _boss._id;
            _channel->Add(ou);
        }
    }
    return HC_SUCCESS;
}

int spls_boss::leave(boost::shared_ptr<CharData>& cdata, boost::shared_ptr<OnlineCharactor>& ou)
{
    if (cdata.get())
    {
        m_char_list.erase(cdata->m_id);
        m_char_enter.erase(cdata->m_id);
        cdata->m_copy_id = 0;
        _channel->Remove(ou);
    }
    return HC_SUCCESS;
}

int spls_boss::total()
{
    return m_char_enter.size();
}

int spls_boss::update(corpsBoss& base, int level)
{
    _boss._base_hp = base.base_hp;
    _boss._hp_per_level = base.hp_every_level;
    _boss._damage_per_level = base.damage_every_level;
    _boss._base_damage1 = base.base_damage1;
    _boss._base_damage2 = base.base_damage2;
    _boss._id = base.id;
    _boss._spic = base.spic;
    _boss._soldier_spic = base.soldier_spic;
    _boss._pufang = base.pufang;
    _boss._cefang = base.cefang;
    _boss._str = base.str;
    _boss._wisdom = base.wisdom;

    strncpy(_boss._name, base.name.c_str(), min(50,(int)base.name.length()));
    strncpy(_boss._chat, base.chat.c_str(), min(200,(int)base.chat.length()));

    _boss._level = level;

    _boss._max_hp = _boss._base_hp + (level - 30) * _boss._hp_per_level;
    _boss._damage1 = _boss._base_damage1 + (level - 30) * base.damage_every_level;
    _boss._damage2 = _boss._base_damage2 + (level - 30) * base.damage_every_level;

    _boss._max_silver = 3000 + (_boss._level - 30) * 100;
    _boss._max_prestige = 30;

    _boss._cur_hp = _boss._max_hp;

    return 0;
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
    q.get_result("select ib.id,ib.spic,ib.soldier_spic,type,ib.name,ib.level,ib.damage1,ib.damage2,ib.max_hp,ib.chat,bb.hp_per_lev,bb.damage_per_lev,bb.pufang,bb.cefang,bb.str,bb.wisdom,bb.state,bb.skill from custom_boss as ib left join base_boss as bb on ib.type=bb.id where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        spls_boss* pb = new spls_boss(id);
        pb->_boss._spic = q.getval();
        pb->_boss._soldier_spic = q.getval();
        pb->_boss._stype = 6;

        pb->_boss._type = q.getval();
        const char* temp = q.getstr();
        if (temp != NULL)
        {
            strncpy(pb->_boss._name, temp, 50);
        }
        pb->_boss._level = q.getval();
        pb->_boss._base_damage1 = q.getval();
        pb->_boss._base_damage2 = q.getval();
        pb->_boss._base_hp = q.getval();
        pb->_boss._cur_hp = pb->_boss._max_hp;
        temp = q.getstr();
        if (temp != NULL)
        {
            strncpy(pb->_boss._chat, temp, 200);
        }
        pb->_boss._hp_per_level = q.getval();
        pb->_boss._damage_per_level = q.getval();
        pb->_boss._pufang = q.getval();
        pb->_boss._cefang = q.getval();
        pb->_boss._str = q.getval();
        pb->_boss._wisdom = q.getval();
        pb->_boss._state = q.getval();
        pb->_boss.m_speSkill = GeneralDataMgr::getInstance()->getSpeSkill(q.getval());

        pb->_boss._damage1 = pb->_boss._base_damage1;
        pb->_boss._damage2 = pb->_boss._base_damage2;
        pb->_boss._max_hp = pb->_boss._base_hp;

        if (pb->_boss._level > 30)
        {
            int fac = pb->_boss._level - 30;
            pb->_boss._damage1 += fac * pb->_boss._damage_per_level;
            pb->_boss._damage2 += fac * pb->_boss._damage_per_level;
            pb->_boss._max_hp += fac * pb->_boss._hp_per_level;
        }

        pb->_boss._max_silver = 3000 + (pb->_boss._level - 30) * 100;
        pb->_boss._max_prestige = 30;

        pb->_boss._last_rule = NULL;

        json_spirit::Object robj;
        getBossActionMemo(pb, robj);

#if 0    //boss没有状态了
        boost::shared_ptr<baseState> bs = baseStateMgr::getInstance()->GetBaseState(pb->_boss._state);
        if (bs.get())
        {
            updateCombatAttribute(&bs, 1, pb->_boss.m_combat_attribute);
        }
#endif
        boost::shared_ptr<spls_boss> spBoss(pb);
        m_boss_maps[pb->_boss._id] = spBoss;
        INFO("############### boss "<<pb->_boss._id<<",hp "<<pb->_boss._max_hp<<endl);
    }
    q.free_result();
    
    std::map<int, boost::shared_ptr<spls_boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        if (it->second.get())
        {
            spls_boss* pb = it->second.get();
            INFO("bossMgr::load,load from custom_shedule"<<pb->_boss._id<<endl);

            q.get_result("select season,day,month,hour,minute,week from custom_shedule where type='openBoss' and param1='" + LEX_CAST_STR(pb->_boss._id) + "'");
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                std::string season = q.getstr();
                std::string day = q.getstr();
                std::string month = q.getstr();
                std::string hour = q.getstr();
                std::string minute = q.getstr();
                std::string week = q.getstr();
                pb->_boss._open_rules.addRule(season, month, day, week, hour, minute);
            }
            q.free_result();
        }
        ++it;
    }

    std::map<int, int> temp_maps;
    //boss單場傷害排名
    q.get_result("select cid,damage from char_boss_damage_rankings where 1 order by damage desc limit " + LEX_CAST_STR(iBossDamageRankingsCount));
    while (q.fetch_row())
    {
        int cid = q.getval();
        int damage = q.getval();
        
        temp_maps[cid] = damage;
    }
    q.free_result();
    m_damage_rankings.updateRankings(temp_maps, "");

    q.get_result("select name,spic,soldier_spic,hp,damage1,damage2,damage_per_lev,hp_per_lev,pufang,cefang,str,wisdom,chat from base_boss where id=3");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_base_corps_boss.id = iCorpsBossId;
        m_base_corps_boss.name = q.getstr();
        m_base_corps_boss.spic = q.getval();
        m_base_corps_boss.soldier_spic = q.getval();
        m_base_corps_boss.base_hp = q.getval();
        m_base_corps_boss.base_damage1 = q.getval();
        m_base_corps_boss.base_damage2 = q.getval();
        m_base_corps_boss.damage_every_level = q.getval();
        m_base_corps_boss.hp_every_level = q.getval();
        m_base_corps_boss.pufang = q.getval();
        m_base_corps_boss.cefang = q.getval();
        m_base_corps_boss.str = q.getval();
        m_base_corps_boss.wisdom = q.getval();
        m_base_corps_boss.chat = q.getstr();
    }
    else
    {
        ERR();
        cout<<"no corps boss data."<<endl;
        assert(false);
    }
    q.free_result();
    
    //加载军团boss
    q.get_result("select corps,open_idx,last_open,level from char_corps_boss where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int corps = q.getval();
        int open_idx = q.getval();
        time_t last_open = q.getval();
        int level = q.getval();

        spls_boss* bs = new spls_boss(iCorpsBossId);
        bs->_corps = corps;
        if (open_idx > iCorpsBossOpenTimeSize || open_idx < 1)
        {
            bs->_corps_hour = 25;
            bs->_corps_min = 0;
        }
        else
        {
            bs->_corps_hour = iCorpsBossOpenTime[open_idx-1][0];
            bs->_corps_min = iCorpsBossOpenTime[open_idx-1][1];
        }
        time_t zero_time = getZeroTime();
        if (last_open < zero_time)
        {
            bs->_corps_state = 0;
        }
        else
        {
            bs->_corps_state = 2;
        }
        bs->_boss._level = level;
        bs->update(m_base_corps_boss, level);
        m_corps_boss[corps].reset(bs);
    }
    q.free_result();

    q.get_result("select id from char_corps where level>=3 and id not in (select corps from char_corps_boss)");
    while (q.fetch_row())
    {
        int id = q.getval();
        addCorpsBoss(id, 0);
    }
    q.free_result();

    return HC_SUCCESS;
}

//鼓舞
int bossMgr::inspire(int cid,int type, json_spirit::Object& robj)
{
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (NULL == pc)
    {
        robj.push_back( Pair("curGx", pc->getGongxun()) );    
        return HC_ERROR;
    }
    int level = m_inspire_map[cid];
    //功勋鼓舞
    if (type == 2)
    {
        if (level >= 5)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_MAX;    //鼓舞顶级了
        }
        //功勋是否足够
        if (pc->addGongxun(-200) < 0)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_NOT_ENOUGH_GONGXUN;
        }
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_gongxun,200,treasure_inspire,2,pc->m_union_id,pc->m_server_id);
        add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_gongxun,200,treasure_inspire_boss,2,pc->m_union_id,pc->m_server_id);
        pc->NotifyCharData();
        //有概率
        if (my_random(1,100) <= iInspireSilver[level])
        {
            m_inspire_map[cid] = level + 1;
        }
        else
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_FAIL;    //失败了
        }
    }
    else
    {
        if (level >= 10)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_MAX;    //鼓舞顶级了
        }
        if (pc->m_vip < iInspireGoldVIPLevel)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_MORE_VIP_LEVEL;
        }
        //金币是否足够
        if (pc->addGold(-20) < 0)
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }
        //金币消耗统计
        //add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 20, gold_cost_for_inspire, pc->m_union_id, pc->m_server_id);
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 20, gold_cost_for_inspire_boss, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc,20,gold_cost_for_inspire_boss);
#endif
        pc->NotifyCharData();
        int rands[] = {100,100,100,100,100,30,20,10,5,5};
        //有概率
        if (my_random(1,100) <= rands[level])
        {
            m_inspire_map[cid] = level + 1;
        }
        else
        {
            robj.push_back( Pair("curGx", pc->getGongxun()) );    
            return HC_ERROR_BOSS_INSPIRE_FAIL;    //失败了
        }
    }
    if (m_inspire_map[cid] > 5)
    {
        pc->m_combat_attribute.boss_inspired((m_inspire_map[cid] - 5)*10 + 100);
    }
    else
    {
        pc->m_combat_attribute.boss_inspired(m_inspire_map[cid]*20);
    }

    robj.push_back( Pair("level", m_inspire_map[cid]) );
    robj.push_back( Pair("damage", pc->m_combat_attribute.boss_inspired()) );

#if 0
    //cout<<"boss inspire "<<pc->m_combat_attribute.m_boss_inspired<<endl;
    robj.push_back( Pair("silver", iInspireSilverCost*pc->m_level) );
    ++level;
    if (level >= 5)
    {
        robj.push_back( Pair("gold", iInspireGoldCost[4]) );
    }
    else
    {
        robj.push_back( Pair("gold", iInspireGoldCost[level]) );
    }
#else
    robj.push_back( Pair("gold", 20) );
    robj.push_back( Pair("gx", 200) );
    robj.push_back( Pair("curGx", pc->getGongxun()) );    
#endif
    std::string suc_msg = strInspireSuccess;
    if (m_inspire_map[cid] > 5)
    {
        str_replace(suc_msg, "$N", LEX_CAST_STR(10));
    }
    else
    {
        str_replace(suc_msg, "$N", LEX_CAST_STR(20));
    }
    robj.push_back( Pair("msg", suc_msg) );
    return HC_SUCCESS;
}

//查询鼓舞等级
int bossMgr::getInspire(int cid, int hlevel, json_spirit::Object& robj)
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
    robj.push_back( Pair("curGx", pc->getGongxun()) );
    robj.push_back( Pair("level", m_inspire_map[cid]) );
    robj.push_back( Pair("damage", pc->m_combat_attribute.boss_inspired()) );
    robj.push_back( Pair("gold", 20) );
    robj.push_back( Pair("gx", 200) );
    return HC_SUCCESS;
}

//清除鼓舞效果
int bossMgr::clearInspire(int cid)
{
    m_inspire_map.erase(cid);
    return 0;
}

//找找boss
boost::shared_ptr<spls_boss> bossMgr::getBoss(int id, int cid)
{
    if (id != iCorpsBossId)
    {
        std::map<int,boost::shared_ptr<spls_boss> >::iterator it = m_boss_maps.find(id);
        if (it != m_boss_maps.end())
        {
            return it->second;
        }
    }
    else
    {
        CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
        if (pc && pc->m_corps_member.get())
        {
            std::map<int,boost::shared_ptr<spls_boss> >::iterator it = m_corps_boss.find(pc->m_corps_member->corps);
            if (it != m_corps_boss.end())
            {
                return it->second;
            }
        }
    }
    boost::shared_ptr<spls_boss> boss;
    return boss;
}

//攻击boss
int bossMgr::Attack(int cid, int id, int type, json_spirit::Object& robj)
{
    spls_boss* pb = getBoss(id, cid).get();
    if (NULL == pb)
    {
        return HC_ERROR;
    }
    int cool = 0;
    int ret = pb->canAttack(cid, cool);
    if (HC_SUCCESS != ret)
    {
        if (cool > 0)
        {
            robj.push_back( Pair("cool", cool) );
        }
        return ret;
    }
    //日常任务
    CharData* pc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
    if (pc)
    {
        if (id == 1)
        {
            dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_attak_boss1);
        }
        else if (id == 4)
        {
            dailyTaskMgr::getInstance()->updateDailyTask(*pc,daily_task_attak_boss4);
        }
    }
    return pb->tryAttack(cid, type, robj);
}

int bossMgr::getBossHp(int cid, int id, json_spirit::Object& robj)
{
    robj.push_back( Pair("id", id) );
    boost::shared_ptr<spls_boss> spb = getBoss(id, cid);
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

int bossMgr::combatResult(Combat* pCombat)    //boss战斗结束
{
    if (NULL == pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    spls_boss* pb = getBoss(pCombat->m_defender->getCharId(), pCombat->m_attacker->getCharId()).get();
    if (NULL == pb)
    {
        ERR();
        return HC_ERROR;
    }
    return pb->combatResult(pCombat);
}

int bossMgr::openBoss(int id, int last_mins)
{
    boost::shared_ptr<spls_boss> spb = getBoss(id, 0);
    if (spb.get())
    {
        spb->open(last_mins);
    }
    return HC_SUCCESS;
}

int bossMgr::closeBoss(int id, int corps)
{
    INFO("close boss "<<id<<endl);

    if (id != iCorpsBossId)
    {
        boost::shared_ptr<spls_boss> spb = getBoss(id, 0);
        if (spb.get())
        {
            spb->close(0, 0, 0);
        }
    }
    else
    {
        std::map<int,boost::shared_ptr<spls_boss> >::iterator it = m_corps_boss.find(corps);
        if (it != m_corps_boss.end())
        {
            if (it->second.get())
            {
                it->second->close(0, 0, 0);
            }
        }
    }
    return HC_SUCCESS;
}

int bossMgr::getBossList(CharData* pc, json_spirit::Array& blist)
{
    //等级不足的不显示boss战图标
    if (!pc || (!pc->m_bossOpen))
    {
        return 0;
    }
    //第一闪
    int first_view = pc->queryExtraData(char_data_type_normal, char_data_view_boss);
    if (0 == first_view)
    {
        pc->setExtraData(char_data_type_normal, char_data_view_boss, 1);
        int state = pc->getDailyState();
        if (0 == state)
        {
            pc->notifyEventState(top_level_event_daily, 0, 0);
        }
    }
    INFO(" ######################### create time "<<pc->m_createTime<<", time(NULL)"<<time(NULL)<<",left "<<leftTime<<endl);

    time_t timep;
    struct tm m_timenow;
    time(&timep);
    localtime_r(&timep, &m_timenow);

    int season = 0;
    boost::shared_ptr<DateInfo> date = GeneralDataMgr::getInstance()->GetDate();
    if (date.get())
    {
        season = date->season;
    }
    else
    {
        ERR();
    }

    //今天要开放的boss列表
    std::map<int, boost::shared_ptr<spls_boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        spls_boss* pb = it->second.get();
        if (pb)
        {
            openRule* pOpenRule = pb->_boss._open_rules.getRule(m_timenow, season);
            if (pb->_boss._open_state || pOpenRule)
            {
                //cout<<pb->_boss._id<<" boss state "<<pb->_boss._open_state<<",";
                json_spirit::Object obj;
                int enable = 0, silver = 0, prestige = 0;
                pb->m_stand_in_mob.getStandIn(pc->m_id, enable, silver, prestige);
                obj.push_back( Pair("baby_state", enable) );
                obj.push_back( Pair("baby_silver", silver) );
                obj.push_back( Pair("baby_prestige", prestige) );
                obj.push_back( Pair("baby_fail", pb->m_damage_maps[pc->m_id] > 0) );
                obj.push_back( Pair("type", action_boss) );
                obj.push_back( Pair("name", pb->_boss._name) );
                obj.push_back( Pair("level", pb->_boss._level) );
                obj.push_back( Pair("spic", pb->_boss._spic) );
                obj.push_back( Pair("id", pb->_boss._id) );
                if (pb->_boss._open_state)
                {
                    obj.push_back( Pair("state", 1) );
                }
                else
                {
                    if ((pOpenRule->_open_hour > m_timenow.tm_hour
                    || (pOpenRule->_open_hour == m_timenow.tm_hour && pOpenRule->_open_min > m_timenow.tm_min)))
                    {
                        obj.push_back( Pair("state", 0) );
                    }
                    else
                    {
                        obj.push_back( Pair("state", 2) );
                    }
                }
                if (pOpenRule)
                {
                    std::string startTime = time2string(pOpenRule->_open_hour, pOpenRule->_open_min);
                    obj.push_back( Pair("startTime", startTime) );
                    //cout<<"open at "<<startTime<<",";

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

int getBossActionMemo(spls_boss* pb, json_spirit::Object& robj)
{    
    if (pb->_boss._open_state)
    {
        robj.push_back( Pair("memo", strBossOpening) );
    }
    else
    {
        time_t timep;
        struct tm m_timenow;
        time(&timep);
        localtime_r(&timep, &m_timenow);

        int season = 0;
        boost::shared_ptr<DateInfo> date = GeneralDataMgr::getInstance()->GetDate();
        if (date.get())
        {
            season = date->season;
        }
        else
        {
            ERR();
        }
        openRule* pOpenRule = pb->_boss._open_rules.getRule(m_timenow, season);
        if (pOpenRule)
        {
            //INFO("################# "<<pb->_boss._open_hour<<":"<<pb->_boss._open_min<<"|"<<m_timenow.tm_hour<<":"<<m_timenow.tm_min<<endl);
            //即将开启
            if ((pOpenRule->_open_hour > m_timenow.tm_hour
                    || (pOpenRule->_open_hour == m_timenow.tm_hour && pOpenRule->_open_min > m_timenow.tm_min)))
            {
                if (pb->_boss._last_rule != pOpenRule)
                {
                    pb->_boss_will_open = strBossWillOpen;
                    pb->_boss_open_time = "";
                    if (pOpenRule->_open_hour < 10)
                    {
                        str_replace(pb->_boss_will_open, "$H", "0" + LEX_CAST_STR(pOpenRule->_open_hour));
                        pb->_boss_open_time = "0" + LEX_CAST_STR(pOpenRule->_open_hour);
                    }
                    else
                    {
                        str_replace(pb->_boss_will_open, "$H", LEX_CAST_STR(pOpenRule->_open_hour));
                        pb->_boss_open_time = LEX_CAST_STR(pOpenRule->_open_hour);
                    }
                    if (pOpenRule->_open_min < 10)
                    {
                        str_replace(pb->_boss_will_open, "$M", "0" + LEX_CAST_STR(pOpenRule->_open_min));
                        pb->_boss_open_time += ":0" + LEX_CAST_STR(pOpenRule->_open_min);
                    }
                    else
                    {
                        str_replace(pb->_boss_will_open, "$M", LEX_CAST_STR(pOpenRule->_open_min));
                        pb->_boss_open_time += ":0" + LEX_CAST_STR(pOpenRule->_open_min);
                    }
                }                        
                robj.push_back( Pair("memo", pb->_boss_will_open) ); 
            }                        
            else
            {
                //结束了
                robj.push_back( Pair("memo", strBossIsClosed) );
            }
            pb->_boss._last_rule = pOpenRule;
        }
        else
        {
            return HC_ERROR;
        }
    }
    return HC_SUCCESS;
}

int bossMgr::getActionMemo(int type, int id, json_spirit::Object& robj)
{
    if (type == 2)
    {
        //今天要开放的boss列表
        std::map<int, boost::shared_ptr<spls_boss> >::iterator it = m_boss_maps.find(id);
        if (it != m_boss_maps.end())
        {
            spls_boss* pb = it->second.get();
            if (pb)
            {
                int ret = getBossActionMemo(pb, robj);
                if (HC_SUCCESS != ret)
                {
                    return ret;
                }                
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
    }
    else
    {
        //robj.push_back( Pair("memo", strNewbieEventMemo) );
    }
    return HC_SUCCESS;
}

void bossMgr::setLevel(int level)    //设置所有boss等级
{
    std::map<int, boost::shared_ptr<spls_boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        spls_boss* pb = it->second.get();
        if (pb)
        {
            pb->_boss._level = level;
            pb->_boss._damage1 = pb->_boss._base_damage1;
            pb->_boss._damage2 = pb->_boss._base_damage2;
            pb->_boss._max_hp = pb->_boss._base_hp;

            if (pb->_boss._level > 30)
            {
                int fac = pb->_boss._level - 30;
                pb->_boss._damage1 += fac * pb->_boss._damage_per_level;
                pb->_boss._damage2 += fac * pb->_boss._damage_per_level;
                pb->_boss._max_hp += fac * pb->_boss._hp_per_level;
            }
            InsertSaveDb("update custom_boss set level=" + LEX_CAST_STR(pb->_boss._level)
                    + " where id=" + LEX_CAST_STR(pb->_boss._id)
                );
        }
        ++it;
    }
}

void bossMgr::updateRankings(std::map<int, int>& dm, const std::string& bossname)
{
    m_damage_rankings.updateRankings(dm, bossname);
}

std::string bossMgr::getRankings(int page)
{
    return m_damage_rankings.getRankings(page);
}

//更新排行榜活动中的cid字段
void bossMgr::updateRankingsEvent(rankings_event* pE)
{
    m_damage_rankings.updateRankingsEvent(pE);
}

bool bossMgr::isOpen()    //boss战是否开启
{
    //今天要开放的boss列表
    std::map<int, boost::shared_ptr<spls_boss> >::iterator it = m_boss_maps.begin();
    while (it != m_boss_maps.end())
    {
        spls_boss* pb = it->second.get();
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

//检查军团boss开放
void bossMgr::checkCorpsBossOpen(int hour, int min)
{
    for (std::map<int, boost::shared_ptr<spls_boss> >::iterator it = m_corps_boss.begin(); it != m_corps_boss.end(); ++it)
    {
        spls_boss& bs = *(it->second.get());
        if (bs._corps_state == 0
            && (hour > bs._corps_hour || (hour == bs._corps_hour && min >= bs._corps_min)))
        {
            bs.open(120);
        }
    }
}

int bossMgr::queryCorpsBossOpen(int corps, json_spirit::Object& robj)
{
    if (m_corps_boss.find(corps) == m_corps_boss.end())
    {
        return HC_ERROR;
    }
    int sel = 0;
    json_spirit::Array list;
    for (int i = 0; i < iCorpsBossOpenTimeSize; ++i)
    {
        json_spirit::Object o;
        o.push_back( Pair("h", iCorpsBossOpenTime[i][0]) );
        o.push_back( Pair("m", iCorpsBossOpenTime[i][1]) );
        list.push_back(o);

        if (m_corps_boss[corps]->_corps_hour == iCorpsBossOpenTime[i][0]
            && m_corps_boss[corps]->_corps_min == iCorpsBossOpenTime[i][1])
        {
            sel = i + 1;
        }
    }
    robj.push_back( Pair("list", list) );
    robj.push_back( Pair("sel", sel) );
    return HC_SUCCESS;
}

//军团boss开启状态
int bossMgr::getJtBossState(int corps)
{
    if (m_corps_boss.find(corps) == m_corps_boss.end())
    {
        return 0;
    }
    return m_corps_boss[corps]->_corps_state;
}

void bossMgr::resetJtBoss(int corps)
{
    //所有军团重置
    if (corps == 0)
    {
        for (std::map<int,boost::shared_ptr<spls_boss> >::iterator it = m_corps_boss.begin(); it != m_corps_boss.end(); ++it)
        {
            (it->second).get()->_corps_state = 0;            
        }
        InsertSaveDb("update char_corps_boss set last_open=" + LEX_CAST_STR(0) + " where 1");
    }
    else
    {
        if (m_corps_boss.find(corps) == m_corps_boss.end())
        {
            return;
        }
        m_corps_boss[corps]->_corps_state = 0;
        InsertSaveDb("update char_corps_boss set last_open=" + LEX_CAST_STR(0) + " where corps=" + LEX_CAST_STR(corps));
    }
}

//设置军团boss开启时间
int bossMgr::setBossOpenTime(int corps, int sel)
{
    //cout<<"set jt boss open time "<<corps<<","<<sel<<endl;
    if (m_corps_boss.find(corps) != m_corps_boss.end())
    {
        if (sel > iCorpsBossOpenTimeSize || sel < 1)
        {
            return HC_ERROR;
        }
        if (m_corps_boss[corps]->_corps_hour != 25 && m_corps_boss[corps]->_corps_state != 2)
        {
            return HC_ERROR_CORPS_BOSS_NOT_END;
        }
        m_corps_boss[corps]->_corps_hour = iCorpsBossOpenTime[sel-1][0];
        m_corps_boss[corps]->_corps_min = iCorpsBossOpenTime[sel-1][1];

        //保存
        InsertSaveDb("update char_corps_boss set open_idx=" + LEX_CAST_STR(sel) + " where corps=" + LEX_CAST_STR(corps));
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//增加军团boss(升级到3级)
int bossMgr::addCorpsBoss(int corps, int idx)
{
    if (m_corps_boss.find(corps) == m_corps_boss.end())
    {
        m_corps_boss[corps].reset(new spls_boss(iCorpsBossId));
        m_corps_boss[corps]->_corps = corps;
        if (idx > iCorpsBossOpenTimeSize || idx < 1)
        {
            m_corps_boss[corps]->_corps_hour = 25;
            m_corps_boss[corps]->_corps_min = 0;
        }
        else
        {
            m_corps_boss[corps]->_corps_hour = iCorpsBossOpenTime[idx-1][0];
            m_corps_boss[corps]->_corps_min = iCorpsBossOpenTime[idx-1][1];
        }

        m_corps_boss[corps]->_corps_state = 0;
        m_corps_boss[corps]->update(m_base_corps_boss, 30);

        //保存
        InsertSaveDb("insert into char_corps_boss (corps,open_idx,level,last_open) values (" + LEX_CAST_STR(corps)
            + "," + LEX_CAST_STR(idx) + ",30,0)");
    }
    return 0;
}

void notifyBossStandIn(session_ptr& psession, int cid, boost::shared_ptr<spls_boss> spb)
{
    //发送替身娃娃相关信息
    int enable = 0, silver = 0, prestige = 0;
    spb->m_stand_in_mob.getStandIn(cid, enable, silver, prestige);
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "getBabyInfo") );
    obj.push_back( Pair("s", 200) );
    
    json_spirit::Object info;
    info.push_back( Pair("state", enable) );
    info.push_back( Pair("silver", silver) );
    info.push_back( Pair("gold", 30) );
    info.push_back( Pair("prestige", prestige) );

    json_spirit::Object action;
    action.push_back( Pair("type", action_boss) );
    action.push_back( Pair("id", spb->_boss._id) );
    action.push_back( Pair("spic", spb->_boss._spic) );
    if (spb->_boss._open_state)
    {
        action.push_back( Pair("active", 1) );
    }
    action.push_back( Pair("openLevel", 0) );
    action.push_back( Pair("openTime", spb->_boss_open_time) );

    info.push_back( Pair("action", action) );
    obj.push_back( Pair("info", info) );
    psession->send(write(obj, json_spirit::raw_utf8));
}

//进入boss场景
int ProcessEnterBossScene(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    if (id != iCorpsBossId && !pc->m_bossOpen)
    {
        return HC_ERROR_BOSS_NOT_ENOUGH_LEVEL;
    }
    //军团boss需要加入12小时才可以
    if (id == iCorpsBossId)
    {
        if (!pc->m_corps_member.get())
        {
            return HC_ERROR_NOT_JOIN_JT;
        }
        if (time(NULL) <= (43200+pc->m_corps_member->join_time))
        {
            return HC_ERROR_CORPS_NEED_12H;
        }
    }
    boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(id, pc->m_id);
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
        robj.push_back( Pair("new_player", cdata->isNewPlayer() > 0) );
        robj.push_back( Pair("new_player_time", cdata->isNewPlayer()) );
        if (spb->_boss._open_state == 0)
        {
            #if 0
            // vip 4以上可以使用替身娃娃
            if (pc->m_vip >= 4)
            {
                //发送替身娃娃相关信息
                notifyBossStandIn(psession, cdata->m_id, spb);
                return HC_SUCCESS_NO_RET;
            }
            else
            #endif
            {
                return HC_ERROR_BOSS_NOT_OPEN;
            }
        }
        #if 0
        else if (pc->m_vip >= 4)
        {
            int enable = 0, silver = 0, prestige = 0;
            bossMgr::getInstance()->m_stand_in_mob.getStandIn(cdata->m_id, enable, silver, prestige);
            if (enable == 1)
            {
                //发送替身娃娃相关信息
                notifyBossStandIn(psession, cdata->m_id, spb);
                return HC_SUCCESS_NO_RET;
            }
        }
        #endif
        if (pc->m_copy_id == id)
        {
            return HC_SUCCESS;
        }
        if (pc->m_copy_id > 0)
        {
            boost::shared_ptr<spls_boss> spbb = bossMgr::getInstance()->getBoss(pc->m_copy_id, pc->m_id);
            if (spbb.get())
            {
                spbb->leave(cdata, account->m_onlineCharactor);
            }
        }
        spb->enter(cdata, account->m_onlineCharactor);
    }
    else
    {
        spb->leave(cdata, account->m_onlineCharactor);
    }
    //广播boss战人数
    {
        std::string msg = spb->_boss_person_msg;
        str_replace(msg, "$P", LEX_CAST_STR(spb->total()));
        spb->_channel->BroadMsg(msg);
    }
    pc->m_copy_id_leave = 0;
    return HC_SUCCESS;
}

//获取Boss固定信息
int ProcessGetBossInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(id, pc->m_id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    boss.push_back( Pair("id", spb->_boss._id) );
    boss.push_back( Pair("name", spb->_boss._name) );
    boss.push_back( Pair("id", spb->_boss._id) );
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

#ifdef TEST_SERVER
    int wait_time = spb->_start_time + 10 - time(NULL);
#else
    int wait_time = spb->_start_time + iBossCombatDelay - time(NULL);
#endif
    info.push_back( Pair("waitTime", wait_time > 0 ? wait_time : 0) );    
    info.push_back( Pair("id", spb->_boss._id) );
    bossMgr::getInstance()->getInspire(pc->m_id, pc->m_level, info);
    robj.push_back( Pair("info", info) );

    return HC_SUCCESS;
}

//获取Boss剩余血量
int ProcessGetBossHp(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    return bossMgr::getInstance()->getBossHp(pc->m_id, id, robj);
}

//获取Boss战参加人数
int ProcessGetBossPerson(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(id, cdata->m_id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("person", spb->total()) );
    return HC_SUCCESS;
}

//鼓舞
int ProcessInspire(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int type = 2;
    READ_INT_FROM_MOBJ(type,o,"type");
    return bossMgr::getInstance()->inspire(pc->m_id, type, robj);
}

//getBossCoolTime //获取攻打Boss冷却时间
int ProcessGetCoolTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(id, pc->m_id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    spls_boss* pb = spb.get();
    int cd = pb->m_attack_cd_maps[pc->m_id] - time(NULL);
    if (cd < 0)
    {
        cd = 0;
    }
    robj.push_back( Pair("time", cd) );
    cd = pb->m_attack_min_cd_maps[pc->m_id] - time(NULL);
    if (cd < 0)
    {
        cd = 0;
    }
    robj.push_back( Pair("time2", cd) );
    int cost_gold = 5 * (pb->m_attack_end_cd_times[pc->m_id]+1);
    robj.push_back( Pair("end_cd_times", iBossEndCDTimes - pb->m_attack_end_cd_times[pc->m_id]) );
    robj.push_back( Pair("end_cd_cost", cost_gold) );
    return HC_SUCCESS;
}

//cmd:endCoolTime //结束攻打Boss冷却时间
int ProcessEndCoolTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    int id = 0, type = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(type,o,"type");//type=1普通秒CD,type=2涅槃
    boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(id, pc->m_id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    spls_boss* pb = spb.get();
    if (type == 1)
    {
        int cd = pb->m_attack_cd_maps[pc->m_id] - time(NULL);
        if (cd > 0)
        {
            if (pc->addGold(-2) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, 2, gold_cost_for_clear_boss_cd, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(pc,2,gold_cost_for_clear_boss_cd);
#endif
            pc->NotifyCharData();
            pb->m_attack_cd_maps[pc->m_id] = 0;
        }
    }
    else if(type == 2)
    {
        int cd = pb->m_attack_min_cd_maps[pc->m_id] - time(NULL);
        if (cd > 0)
        {
            if (pb->m_attack_end_cd_times[pc->m_id] >= iBossEndCDTimes)
            {
                return HC_ERROR;
            }
            int cost_gold = 5 * (pb->m_attack_end_cd_times[pc->m_id]+1);
            if (pc->addGold(-cost_gold) < 0)
            {
                return HC_ERROR_NOT_ENOUGH_GOLD;
            }
            //金币消耗统计
            add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, cost_gold, gold_cost_for_clear_boss_cd_best, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
            gold_cost_tencent(pc,cost_gold,gold_cost_for_clear_boss_cd_best);
#endif
            pc->NotifyCharData();
            pb->m_attack_min_cd_maps[pc->m_id] = 0;
            pb->m_attack_cd_maps[pc->m_id] = 0;
            pb->m_attack_end_cd_times[pc->m_id] = pb->m_attack_end_cd_times[pc->m_id] + 1;
        }
    }
    return HC_SUCCESS;
}

//cmd:getBossDamage//获取对Boss造成伤害的玩家列表
int ProcessGetBossDamage(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    boost::shared_ptr<spls_boss> spb = bossMgr::getInstance()->getBoss(id, pc->m_id);
    if (!spb.get())
    {
        return HC_ERROR;
    }
    
    json_spirit::Array dlist;
    spls_boss* pb = spb.get();
    robj.push_back( Pair("damage", pb->m_damage_maps[pc->m_id]) );
    robj.push_back( Pair("percent", get_percent(pb->m_damage_maps[pc->m_id], pb->_boss._max_hp)) );

    int rank = 0;
    for (std::list<bossDamage>::iterator it = pb->_topten_damages.begin(); it != pb->_topten_damages.end(); ++it)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("rank", ++rank) );
        obj.push_back( Pair("name", it->name) );
        obj.push_back( Pair("damage", it->damage) );
        obj.push_back( Pair("percent", it->percent) );
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

//cmd:getBossRankList//获取对Boss造成最高伤害的玩家排行榜
int ProcessGetBossRankList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int page = 1;
    READ_INT_FROM_MOBJ(page, o, "page");
    std::string msg = bossMgr::getInstance()->getRankings(page);
    psession->send(msg);
    return HC_SUCCESS_NO_RET;
}

//cmd:queryJtBossTime
int ProcessQueryJtBossTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (pc->m_corps_member.get())
    {
        return bossMgr::getInstance()->queryCorpsBossOpen(pc->m_corps_member->corps, robj);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

//cmd:setJtBossTime
int ProcessSetJtBossTime(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (pc->m_corps_member.get())
    {
        if (pc->m_corps_member->offical != 1)
        {
            return HC_ERROR_CORPS_OFFICAL_LIMIT;
        }
        int sel = 1;
        READ_INT_FROM_MOBJ(sel, o, "sel");
        return bossMgr::getInstance()->setBossOpenTime(pc->m_corps_member->corps, sel);
    }
    else
    {
        return HC_ERROR_NOT_JOIN_JT;
    }
}

//cmd:setJtBossTime
int ProcessCheckJtBossOpen(json_spirit::mObject& o)
{
    int hour = 8, min = 0;
    READ_INT_FROM_MOBJ(hour, o, "hour");
    READ_INT_FROM_MOBJ(min, o, "min");

    bossMgr::getInstance()->checkCorpsBossOpen(hour, min);
    return HC_SUCCESS;
}

