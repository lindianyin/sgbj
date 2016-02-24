#include "statistics.h"

#include "army.h"
#include "combat.h"
#include "groupCombat.h"
#include "utils_all.h"
#include "spls_errcode.h"
#include "data.h"
#include "net.h"
#include "groupCopy.h"
#include "loot.h"
#include "daily_task.h"

using namespace json_spirit;
using namespace net;

#define INFO(x) //cout<<x

extern std::string strCombatResultMemo;
extern std::string strCombatResultMemo2;

extern void InsertCombat(Combat* pcombat);
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);
extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int groupCombat::start()
{
    for (int i = 0; i < iMaxGroupCombat; ++i)
    {
        if (_combats[i] == NULL)
        {
            _combats[i] = createCombat(i+1);
        }
    }
    //while (createCombat2())
    //{
    //    ;
    //}
    return HC_SUCCESS;
}

void groupCombat::broadCastMsg(const std::string& msg)
{
    //cout << "groupCombat::broadCastMsg" << endl;
    for (std::list<army_data*>::iterator it = _attackerList.begin(); it != _attackerList.end(); ++it)
    {
        if (0 == (*it)->m_type)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor((*it)->m_name);
            if (account)
            {
                account->Send(msg);
            }
        }
    }
    for (std::list<army_data*>::iterator it = _defenderList.begin(); it != _defenderList.end(); ++it)
    {
        if (0 == (*it)->m_type)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor((*it)->m_name);
            if (account)
            {
                account->Send(msg);
            }
        }
    }
}

Combat* groupCombat::createCombat(int pos, army_data* army_a, army_data* army_d)
{
    Army* attacker = new Army(army_a);
    Army* defender = new Army(army_d);
    Combat* pCombat = new Combat(attacker, defender);

    pCombat->m_type = _combat_type;
    pCombat->m_type_id = _combat_id;
    pCombat->m_position = pos;
    attacker->setCombat(pCombat);
    defender->setCombat(pCombat);

    //清除上一场战斗遗留的buff
    pCombat->m_attacker->clearBuff();

    //战斗双方数据信息
    json_spirit::Object aobj;
    pCombat->m_attacker->GetObj(aobj);
    json_spirit::Object dobj;
    pCombat->m_defender->GetObj(dobj);
    
    pCombat->m_combat_info.push_back( Pair("attackList", aobj) );
    pCombat->m_combat_info.push_back( Pair("defenseList", dobj) );
    
    pCombat->m_combat_id = GeneralDataMgr::getInstance()->newCombatId();
    
    pCombat->m_combat_info.push_back( Pair("battleId", pCombat->m_combat_id) );
    pCombat->m_combat_info.push_back( Pair("type", pCombat->m_type) );
    pCombat->m_combat_info.push_back( Pair("position", pCombat->m_position) );

    //立即返回战斗双方信息
    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "attack") );
    robj.push_back( Pair("s", 200) );
    robj.push_back( Pair ("getBattleList", pCombat->getCombatInfo()));
    std::string msg = json_spirit::write(robj);
    broadCastMsg(msg);

    INFO(" !!!!!!!!!!!!!!!!!!!!!!!! groupCombat creat combat at pos "<<pos<<endl);
    //战斗过程处理
    InsertCombat(pCombat);
    return pCombat;
}

Combat* groupCombat::createCombat(int pos)
{
    INFO("********* createCombat() at pos "<<pos<<endl);
    //本列还没打完，先打完本列的
    if (_attackerQues[pos-1].size() > 0 && _defenderQues[pos-1].size() > 0)
    {
        army_data* army_a = *(_attackerQues[pos-1].begin());
        _attackerQues[pos-1].pop_front();
        army_data* army_d = *(_defenderQues[pos-1].begin());
        _defenderQues[pos-1].pop_front();

        return createCombat(pos, army_a, army_d);
    }
    
    INFO("********* createCombat() return NULL "<<endl);
    return NULL;
}

Combat* groupCombat::createCombat2(int pos)
{
    INFO("********* createCombat2() "<<pos<<endl);

    //for (int pos = 1; pos <= iMaxGroupCombat; ++pos)
    {
        //本列没有战斗发生
        if (!_combats[pos-1])
        {
            INFO("********* createCombat2() pos "<<pos<<endl);

            army_data* army_a = NULL;
            int pos_a = 0;
            if (_attackerQues[pos-1].size() > 0)
            {
                army_a = *(_attackerQues[pos-1].begin());
                pos_a = pos;
                INFO("********* createCombat2() pos "<<pos<<",get attacker at pos "<<pos_a<<endl);
            }
            else
            {
                for (int i = 0; i < iMaxGroupCombat; ++i)
                {
                    if (!_combats[i] && _attackerQues[i].size() > 0)
                    {
                        pos_a = i + 1;
                        army_a = *(_attackerQues[i].begin());
                        INFO("********* createCombat2() pos "<<pos<<",get attacker at pos "<<pos_a<<endl);
                        break;
                    }
                }
            }

            army_data* army_d = NULL;
            int pos_d = 0;
            if (army_a)
            {
                if (_defenderQues[pos-1].size() > 0)
                {
                    army_d = *(_defenderQues[pos-1].begin());
                    pos_d = pos;
                    INFO("********* createCombat2() pos "<<pos<<",get defender at pos "<<pos_d<<endl);
                }
                else
                {
                    for (int i = 0; i < iMaxGroupCombat; ++i)
                    {
                        if (!_combats[i] && _defenderQues[i].size() > 0)
                        {
                            pos_d = i + 1;
                            army_d = *(_defenderQues[i].begin());
                            INFO("********* createCombat2() pos "<<pos<<",get defender at pos "<<pos_d<<endl);
                            break;
                        }
                    }
                }
                if (army_d)
                {
                    _attackerQues[pos_a-1].pop_front();
                    _defenderQues[pos_d-1].pop_front();

                    _combats[pos-1] = createCombat(pos, army_a, army_d);
                    return _combats[pos-1];
                }
            }
            INFO("********* createCombat2() return NULL "<<pos<<endl);
            return NULL;
        }
    }

    INFO("********* createCombat2() return NULL "<<endl);

    return NULL;
}

int groupCombat::onCombatResult(Combat* pCombat)
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    //发送战报
    if (pCombat->m_final_result != "")
    {
        broadCastMsg(pCombat->m_final_result);
    }

    //战斗结果
    if (attacker_win == pCombat->m_state)
    {
        pCombat->m_attacker->_army_data->_update();
        if (pCombat->m_position >= 1 && pCombat->m_position <= iMaxGroupCombat)
        {
            _attackerQues[pCombat->m_position-1].push_back(pCombat->m_attacker->_army_data);
        }
        else
        {
            _attackerQues[0].push_back(pCombat->m_attacker->_army_data);
        }
    }
    else
    {
        pCombat->m_defender->_army_data->_update();
        if (pCombat->m_position >= 1 && pCombat->m_position <= iMaxGroupCombat)
        {
            _defenderQues[pCombat->m_position-1].push_back(pCombat->m_defender->_army_data);
        }
        else
        {
            _defenderQues[0].push_back(pCombat->m_defender->_army_data);
        }
    }
    INFO("groupCombat on combat result. at pos "<<pCombat->m_position<<",result "<<pCombat->m_state<<endl);

    if (pCombat->m_position >= 1 && pCombat->m_position <= iMaxGroupCombat)
    {
        _combats[pCombat->m_position-1] = NULL;
        createCombat2(pCombat->m_position);
    }
    //下一场战斗
    #if 0
    for (int i = 0; i < iMaxGroupCombat; ++i)
    {
        if (_combats[i] == NULL || _combats[i] == pCombat)
        {
            _combats[i] = createCombat(i+1);
        }
    }
    while (createCombat2())
    {
        ;
    }
    #endif
    int state = pCombat->m_state;
    delete pCombat->m_attacker;
    delete pCombat->m_defender;
    delete pCombat;
    bool bIsEnd = true;
    for (int i = 0; i < iMaxGroupCombat; ++i)
    {
        if (_combats[i])
        {
            bIsEnd = false;
            break;
        }
    }
    if (bIsEnd)
    {
        onEnd(state);
        return -1;
    }
    return HC_SUCCESS;
}

groupCombat::groupCombat(int type, int combat_id, int extra_id)
{
    _combat_type = type;
    _combat_id = combat_id;
    _extra_id = extra_id;
    for (int i = 0; i < iMaxGroupCombat; ++i)
    {
        _combats[i] = NULL;    //正在进行的战斗
    }
}

groupCombat::~groupCombat()
{
    for (std::list<army_data*>::iterator it = _attackerList.begin(); it != _attackerList.end(); ++it)    //攻击方队伍
    {
        delete *it;
    }
    for (std::list<army_data*>::iterator it = _defenderList.begin(); it != _defenderList.end(); ++it)    //防守方队伍
    {
        delete *it;
    }
}

void groupCombat::setAttacker(const std::list<army_data *> & a)
{
    _attackerList = a;
    std::list<army_data *>::const_iterator ita = a.begin();

    while (ita != a.end())
    {
        for (int i = 0; i < iMaxGroupCombat; ++i)
        {
            //每列一个
            _attackerQues[i].push_back(*ita);
            ++ita;
            if (ita == a.end())
            {
                return;
            }
        }        
        --ita;
    }
}

void groupCombat::setDefender(const std::list<army_data *> & a)
{
    _defenderList = a;
    std::list<army_data *>::const_iterator ita = a.begin();
    while (ita != a.end())
    {
        for (int i = 0; i < iMaxGroupCombat; ++i)
        {
            //每列2个
            _defenderQues[i].push_back(*ita);
            ++ita;
            if (ita != a.end())
            {
                _defenderQues[i].push_back(*ita);
            }
            else
            {
                return;
            }
            ++ita;
            if (ita == a.end())
            {
                return;
            }
        }        
        --ita;
    }
}

groupCopyCombat::groupCopyCombat(int combatid, int copyId, int groupId, const std::list<army_data*>& alist, const std::list<army_data*>& dlist, int* l_bfriend)
:groupCombat(combat_group_copy, combatid, copyId)
{
    INFO("groupCopyCombat::groupCopyCombat()"<<endl);
    _copyid = copyId;
    _combatId = combatid;
    _groupid = groupId;
    setAttacker(alist);
    setDefender(dlist);
    for (int i = 0; i < iMaxGroupCopyMembers; i++)
    {
        _bfriend[i] = l_bfriend[i];
        //cout << "_bfriend["<< i << "]=" << _bfriend[i] << endl;
    }
}

void groupCopyCombat::broadCastMsg(const std::string& msg)
{
    //cout << "groupCopyCombat::broadCastMsg" << endl;
    int cnt = 0;//检查是否好友虚拟
    for (std::list<army_data*>::iterator it = _attackerList.begin(); it != _attackerList.end(); ++it)
    {
        if (0 == (*it)->m_type && _bfriend[cnt] == 0)
        {
            boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor((*it)->m_name);
            if (account)
            {
                account->Send(msg);
            }
        }
        ++cnt;
    }
}

int groupCopyCombat::onEnd(int state)
{
    INFO("groupCopyCombat()::onEnd("<<state<<")"<<endl);

    json_spirit::Object robj;
    robj.push_back( Pair("cmd", "battleResult") );
    robj.push_back( Pair("s", 200) );
    robj.push_back( Pair("type", combat_group_copy) );
    robj.push_back( Pair("result", state == attacker_win ? 3 : 8) );

    std::string memo = state == attacker_win ?strCombatResultMemo : strCombatResultMemo2;
    //str_replace(memo, "$A", m_attacker->Name());
    str_replace(memo, "$D", _npc_name);

    std::list<Item> loots;
    //副本声望
    Item itemPrestige;

    //副本功勋
    Item itemGongxun;
    if (attacker_win == state)
    {
        lootMgr::getInstance()->getGroupCopyLoots(_copyid, loots, 0);
        groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(_copyid);

        itemPrestige.type = item_type_prestige;
        itemPrestige.id = 0;
        itemPrestige.nums = 10;
        if (pCopy)
        {
            itemPrestige.nums = pCopy->_prestige_reward;

            //std::list<army_data*>::iterator it = _attackerList.begin();
            //while (it != _attackerList.end())
            //{
            //    if (*it && 0 == (*it)->m_type)
            //    {
            //        
            //    }
            //    ++it;
            //}
        }
        if (itemPrestige.nums > 0)
        {
            loots.push_back(itemPrestige);
        }

        itemGongxun.type = item_type_treasure;
        itemGongxun.id = treasure_type_gongxun;
        itemGongxun.nums = 0;
        if (pCopy)
        {
            itemGongxun.nums = pCopy->_gongxun_reward;
        }

        loots.push_back(itemGongxun);
    }
    else
    {
        //失敗了返還次數
        groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(_copyid);
        if (pCopy)
        {
            int cnt = 0;
            std::list<army_data*>::iterator it = _attackerList.begin();
            while (it != _attackerList.end())
            {
                if (*it && 0 == (*it)->m_type && _bfriend[cnt] == 0)
                {
                    --pCopy->_attackTimesMaps[(*it)->m_charactor];
                }
                ++cnt;
                ++it;
            }
        }
    }
    std::list<army_data*>::iterator it = _attackerList.begin();
    int cnt = 0;//检查是否好友虚拟
    while (it != _attackerList.end())
    {
        if (*it && 0 == (*it)->m_type)
        {
            CharData* pc = GeneralDataMgr::getInstance()->GetCharData((*it)->m_charactor).get();
            if (pc)
            {
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor((*it)->m_name);
                if (account && _bfriend[cnt] == 0)
                {
                    json_spirit::Object robj2(robj);
                    if (attacker_win == state)
                    {
                        std::list<Item> items = loots;
                        giveLoots(pc, items, pc->m_area, pc->m_level, 0, NULL, &robj2, true, give_groupCombat_loot);
                        if (itemPrestige.nums > 0)
                        {
                            //軍團增加經驗
                            //corpsMgr::getInstance()->addEvent(pc, corps_event_add_exp, itemPrestige.nums, 0);
                        }
                        pc->NotifyCharData();
                    }
                    std::string memo2 = memo;
                    str_replace(memo2, "$A", pc->m_name);
                    robj2.push_back( Pair("memo", memo2));
                    account->Send(write(robj2));
                }
                ++cnt;
            }
        }
        ++it;
    }
    return 0;
}

groupCombatMgr* groupCombatMgr::m_handle = NULL;
groupCombatMgr* groupCombatMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new groupCombatMgr();
    }
    return m_handle;
}

groupCombatMgr::groupCombatMgr()
{
    m_groupCopyCombatId = 1;
}

int groupCombatMgr::combatResult(Combat* pCombat)    //战斗结束
{
    if (!pCombat)
    {
        ERR();
        return HC_ERROR;
    }
    if (combat_group_copy != pCombat->m_type)
    {
        ERR();
        return HC_ERROR;
    }
    std::map<int, boost::shared_ptr<groupCopyCombat> >::iterator it = m_groupCopyCombats.find(pCombat->m_type_id);
    if (it == m_groupCopyCombats.end() || !it->second.get())
    {
        ERR();
        return HC_ERROR;
    }
    groupCopyCombat* pg = it->second.get();
    int ret = pg->onCombatResult(pCombat);
    //整个战斗结束了
    if (ret == -1)
    {
        m_groupCopyCombats.erase(it);
    }
    return ret;
}

//攻击多人副本
int groupCombatMgr::AttackGroupCopy(int groupId, int copyId, int* l_bfriend)
{
    int group_combat_id = ++m_groupCopyCombatId;
    groupCopy* pCopy = groupCopyMgr::getInstance()->getCopy(copyId);
    if (!pCopy)
    {
        return HC_ERROR;
    }
    std::list<army_data*> alist;
    if (HC_SUCCESS != pCopy->loadTeamArmy(groupId, alist))
    {
        return HC_ERROR;
    }
    std::list<army_data*> dlist;
    pCopy->loadNpcArmy(groupId, dlist);
    boost::shared_ptr<groupCopyCombat> spG(new groupCopyCombat(group_combat_id, copyId, groupId, alist, dlist, &l_bfriend[0]));
    spG->npcName(pCopy->_name);
    m_groupCopyCombats[group_combat_id] = spG;
    spG->start();
    return HC_SUCCESS;
}

