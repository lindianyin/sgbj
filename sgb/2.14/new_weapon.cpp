#include "statistics.h"

#include "new_weapon.h"
#include "net.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include "daily_task.h"
#include "singleton.h"
#include "relation.h"

extern Database& GetDb();

using namespace std;
using namespace net;

newWeaponMgr* newWeaponMgr::m_handle = NULL;
newWeaponMgr* newWeaponMgr::getInstance()
{
    if (m_handle == NULL)
    {
        m_handle = new newWeaponMgr();        
        m_handle->reload();
    }
    return m_handle;
}

int newWeaponMgr::reload()
{
    m_max_new_weapon = 0;
    Query q(GetDb());
    q.get_result("select id,openLevel,wType,baseCost,baseEffect,effectPerLevel,maxLevel,qualtity,name,memo from base_new_weapons where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id > 100 || id < 1)
        {
            ERR();
            continue;
        }
        _base_new_weapons[id-1]._id = id;
        _base_new_weapons[id-1]._openStronghold = q.getval();
        _base_new_weapons[id-1]._type = q.getval();
        _base_new_weapons[id-1]._baseCost = q.getval();
        _base_new_weapons[id-1]._baseEffect = q.getval();
        _base_new_weapons[id-1]._effectPerLevel = q.getval();
        _base_new_weapons[id-1]._maxLevel = q.getval();
        _base_new_weapons[id-1]._quality = q.getval();
        _base_new_weapons[id-1]._name = q.getstr();
        _base_new_weapons[id-1]._memo = q.getstr();
        _base_new_weapons[id-1]._mapid = (id - 1) / 5 + 1;

        if (id >= 1 && id <= 5)
        {
            boost::shared_ptr<StrongholdData> strhold = GeneralDataMgr::getInstance()->GetStrongholdData(_base_new_weapons[id-1]._openStronghold);
            if (strhold.get())
            {
                _base_new_weapons[id-1]._openLevel = strhold->m_level;
            }
            else
            {
                ERR();
                exit(0);
            }
        }
        else
        {
            _base_new_weapons[id-1]._openLevel = ((id-1)/5) * 20 + 1;
        }
        m_max_new_weapon = id;
    }
    q.free_result();
    return HC_SUCCESS;
}

baseNewWeapon* newWeaponMgr::getWeapon(int id)
{
    if (id >= 1 && id <= m_max_new_weapon)
    {
        return _base_new_weapons + id - 1;
    }
    return NULL;
}

baseNewWeapon* newWeaponMgr::getDefaultWeapon(int type)
{
    if (type >= 1 && type <= 5)
    {
        return _base_new_weapons + type - 1;
    }
    return NULL;
}

int newWeaponMgr::openLevel()
{
    return openLevel(1);
}

int newWeaponMgr::openLevel(int type)
{
    baseNewWeapon* p = getDefaultWeapon(type);
    if (p)
    {
        return p->_openLevel;
    }
    return 0;
}

int newWeaponMgr::openStronghold()
{
    return openStronghold(1);
}

int newWeaponMgr::openStronghold(int type)
{
    baseNewWeapon* p = getDefaultWeapon(type);
    if (p)
    {
        return p->_openStronghold;
    }
    return 0;
}

int baseNewWeapon::effect(int level)    //根据等级返回加成数值
{
    if (level % 20 != 0)
    {
        level = level % 20;
    }
    else
    {
        level = 20;
    }
    return _baseEffect + _effectPerLevel * (level - 1);
}

int baseNewWeapon::levelCost(int level)
{
    if (level % 20 != 0)
    {
        level = level % 20;
    }
    else
    {
        level = 20;
    }
    int idx = (_id-1)/5;
    if (idx >= 0 && idx <= 5)
    {
        return iNewWeaponUpgrade[idx][level-1];
    }
    else
    {
        return 0;
    }
    //return _baseCost + (level - 1) * _baseCost / (2*_mapid);
}

baseNewWeapon* baseNewWeapon::nextLevel(int level, int& next_level)    //下一级
{
    if (level % _maxLevel != 0)
    {
        next_level = level + 1;
        return this;
    }
    baseNewWeapon* next = newWeaponMgr::getInstance()->getWeapon(_id + 5);
    if (next)
    {        
        next_level = level + 1;
        return next;
    }
    else
    {
        next_level = 0;
        return NULL;
    }
}

//简单信息的列表
void CharNewWeapons::getList(json_spirit::Array& wlist)
{
    static int cmap[] = {0,2,4,1,3};
    for (int j = 0; j < 5; ++j)
    {
        int i = cmap[j];
        if (_weapons[i]._baseWeapon)
        {
            baseNewWeapon* p = _weapons[i]._baseWeapon;
            json_spirit::Object obj;
            obj.push_back( Pair("type", p->_type) );
            obj.push_back( Pair("name", p->_name) );
            obj.push_back( Pair("quality", p->_quality) );
            obj.push_back( Pair("addNums", _weapons[i]._effect) );
            obj.push_back( Pair("spic", p->_id) );
            obj.push_back( Pair("level", _weapons[i]._level) );
            wlist.push_back(obj);
        }
        else
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("needLevel", newWeaponMgr::getInstance()->openLevel(i+1)) );
            wlist.push_back(obj);
        }
    }
}

void CharNewWeapons::updateNewAttack()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(_cid);
    if (!cdata.get())
        return;
    _score = 0;
    _power_pu = 0;
    _power_ce = 0;
    int tmp = 0;
    for (int i = 0; i < 5; ++i)
    {
        tmp += _weapons[i]._level;
    }
    if (cdata->m_level < 13)
    {
        _score = (int)((double)tmp / cdata->m_level / 1.0 * 100.0);
    }
    else if(cdata->m_level < 25)
    {
        _score = (int)((double)tmp / cdata->m_level / 2.0 * 100.0);
    }
    else if(cdata->m_level < 37)
    {
        _score = (int)((double)tmp / cdata->m_level / 3.0 * 100.0);
    }
    else if(cdata->m_level < 45)
    {
        _score = (int)((double)tmp / cdata->m_level / 4.0 * 100.0);
    }
    else
    {
        _score = (int)((double)tmp / cdata->m_level / 5.0 * 100.0);
    }
    _score = _score > 100 ? 100 : _score;
    //0普攻1普防2策攻3策防4兵力
    _power_pu = cdata->buff_attack(act_wuli_attack, _weapons[0]._effect, _weapons[4]._effect, _weapons[1]._effect, _weapons[3]._effect);
    _power_ce = cdata->buff_attack(act_celue_attack, _weapons[2]._effect, _weapons[4]._effect, _weapons[1]._effect, _weapons[3]._effect);
    return;
}

//显示兵器
int ProcessListNewWeapons(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> m_ponlineuser = psession->user();
    if (m_ponlineuser == NULL)
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!m_ponlineuser->m_onlineCharactor.get() || !m_ponlineuser->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = m_ponlineuser->m_onlineCharactor->m_charactor.get();
    if (pc->m_currentStronghold < newWeaponMgr::getInstance()->openStronghold())
    {
        robj.push_back( Pair("open", 0) );
        return HC_SUCCESS;
    }
    robj.push_back( Pair("open", 1) );
    json_spirit::Array wlist;
    static int cmap[] = {0,2,4,1,3};
    for (int j = 0; j < 5; ++j)
    {
        int i = cmap[j];
        if (pc->m_new_weapons._weapons[i]._baseWeapon)
        {
            baseNewWeapon* p = pc->m_new_weapons._weapons[i]._baseWeapon;
            json_spirit::Object obj;
            obj.push_back( Pair("id", p->_id) );
            obj.push_back( Pair("type", p->_type) );
            obj.push_back( Pair("name", p->_name) );
            obj.push_back( Pair("quality", p->_quality) );
            obj.push_back( Pair("addNums", pc->m_new_weapons._weapons[i]._effect) );
            obj.push_back( Pair("spic", p->_id) );
            obj.push_back( Pair("level", pc->m_new_weapons._weapons[i]._level) );
            wlist.push_back(obj);
        }
        else
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("needLevel", newWeaponMgr::getInstance()->openLevel(i+1)) );
            wlist.push_back(obj);
        }
    }
    robj.push_back( Pair("list", wlist) );
    return HC_SUCCESS;
}

//显示兵器描述
int ProcessQueryWeaponMemo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    baseNewWeapon* pn = newWeaponMgr::getInstance()->getWeapon(id);
    if (pn)
    {
        json_spirit::Object wobj;
        wobj.push_back( Pair("memo", pn->_memo) );
        wobj.push_back( Pair("name", pn->_name) );
        wobj.push_back( Pair("type", pn->_type) );
        wobj.push_back( Pair("quality", pn->_quality) );
        wobj.push_back( Pair("spic", pn->_id) );
        robj.push_back( Pair("weaponVO", wobj) );
    }
    return HC_SUCCESS;
}

//显示兵器升级信息
int ProcessQueryWeaponUpgradeInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> m_ponlineuser = psession->user();
    if (m_ponlineuser == NULL)
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!m_ponlineuser->m_onlineCharactor.get() || !m_ponlineuser->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = m_ponlineuser->m_onlineCharactor->m_charactor.get();
    int type = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    if (type > 5 || type < 1)
    {
        ERR();
        return HC_ERROR;
    }
    if (NULL == pc->m_new_weapons._weapons[type-1]._baseWeapon)
    {
        return HC_SUCCESS;
    }
    json_spirit::Object wobj;
    int nextLevel = 0;
    baseNewWeapon* next = pc->m_new_weapons._weapons[type-1]._baseWeapon->nextLevel(pc->m_new_weapons._weapons[type-1]._level, nextLevel);
    if (!next)
    {
        wobj.push_back( Pair("level", 0) );
        wobj.push_back( Pair("disable", 2) );
        wobj.push_back( Pair("memo", pc->m_new_weapons._weapons[type-1]._baseWeapon->_memo) );
        robj.push_back( Pair("weaponVO", wobj) );
        return HC_SUCCESS;
    }
    int needSilver = pc->m_new_weapons._weapons[type-1]._cost;
    needSilver = needSilver < 0 ? 0 : needSilver;
    
    wobj.push_back( Pair("level", nextLevel) );
    wobj.push_back( Pair("memo", pc->m_new_weapons._weapons[type-1]._baseWeapon->_memo) );
    wobj.push_back( Pair("addNums", next->effect(nextLevel)) );
    wobj.push_back( Pair("curGx", pc->getGongxun()) );
    wobj.push_back( Pair("gx", needSilver) );
    if (pc->m_new_weapons._weapons[type-1]._baseWeapon != next)
    {
        wobj.push_back( Pair("upgrade", 1) );
    }
    robj.push_back( Pair("weaponVO", wobj) );

    return HC_SUCCESS;
}

//兵器升级
int ProcessUpgradeWeapon(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    //账号登录状态
    boost::shared_ptr<OnlineUser> m_ponlineuser = psession->user();
    if (m_ponlineuser == NULL)
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    if (!m_ponlineuser->m_onlineCharactor.get() || !m_ponlineuser->m_onlineCharactor->m_charactor.get())
    {
        return HC_ERROR_LOGIN_FIRST;
    }
    CharData* pc = m_ponlineuser->m_onlineCharactor->m_charactor.get();
    int type = 1;
    READ_INT_FROM_MOBJ(type, o, "type");
    if (type > 5 || type < 1)
    {
        ERR();
        return HC_ERROR;
    }
    if (NULL == pc->m_new_weapons._weapons[type-1]._baseWeapon)
    {
        return HC_ERROR;
    }
    int nextLevel = 0;
    baseNewWeapon* next = pc->m_new_weapons._weapons[type-1]._baseWeapon->nextLevel(pc->m_new_weapons._weapons[type-1]._level, nextLevel);
    if (!next)
    {
        return HC_ERROR;
    }
    //升级兵器扣除功勋
    int needGongxun = pc->m_new_weapons._weapons[type-1]._cost;
    needGongxun = needGongxun < 0 ? 0 : needGongxun;
    if (pc->m_level < pc->m_new_weapons._weapons[type-1]._baseWeapon->_openLevel
        || pc->m_level < nextLevel)
    {
        return HC_ERROR_WEAPON_NOT_ENOUGH_LEVEL;
    }
    if (pc->addGongxun(-needGongxun) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GONGXUN;
    }
    add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_gongxun,needGongxun,treasure_weapon,2,pc->m_union_id,pc->m_server_id);
    pc->NotifyCharData();
    pc->m_new_weapons._weapons[type-1]._baseWeapon = next;
    pc->m_new_weapons._weapons[type-1]._level = nextLevel;
    pc->m_new_weapons._weapons[type-1]._effect = next->effect(nextLevel);
    pc->m_new_weapons._weapons[type-1]._cost = next->levelCost(nextLevel);
    pc->SaveWeapons(type);

    pc->updateUpgradeWeaponCost();
    pc->updateUpgradeWeaponCDList();
    
    pc->m_weapon_attack_change = true;    //影響戰力
    pc->m_new_weapons.updateNewAttack();

    //更新任务
    pc->m_trunk_tasks.updateTask(task_weapon_level, next->_id, nextLevel);

    //秘法升级，好友祝贺
    if (nextLevel > 10 && nextLevel % 10 == 0)
    {
        Singleton<relationMgr>::Instance().postCongradulation(pc->m_id, CONGRATULATION_MIFA, next->_id, nextLevel);
    }
    //act统计
    act_to_tencent(pc,act_new_mifa,next->_type);
    return HC_SUCCESS;
}

