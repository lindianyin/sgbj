
#include "training.h"
#include "net.h"
#include "singleton.h"
#include "ThreadLocalSingleton.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "statistics.h"

#include "SaveDb.h"
#include "utils_lang.h"
#include "mails.h"


Database& GetDb();

void InsertSaveDb(const std::string& sql);

void CharTrainings::load(CharData& c)
{
    Query q(GetDb());
    //加载练兵
    q.get_result("select sType,center_id,id_1,id_2,id_3,id_4,id_5 from char_training where cid=" + LEX_CAST_STR(_cid));
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int type = q.getval();
        if (type >= 1 && type <= iTrainingNum)
        {
            m_soldier_souls[type-1].type = type;
            //魂眼
            int center_id = q.getval();
            m_soldier_souls[type-1]._centerSoul = Singleton<trainingMgr>::Instance().getSoul(center_id);
            //兵魂
            for (int stype = 1; stype <= iSoulNum; ++stype)
            {
                int tmp_id = q.getval();
                m_soldier_souls[type-1]._soul[stype-1] = Singleton<trainingMgr>::Instance().getSoul(tmp_id);
            }
        }
        else
        {
            ERR();
        }
    }
    q.free_result();
    checkSouls(c);
    updateAttribute(c);
    return;
}

int CharTrainings::getNewScore()
{
    return _score;
}

void CharTrainings::checkSouls(CharData& c)
{
    for (int type = 1; type <= iTrainingNum; ++type)
    {
        if (!m_soldier_souls[type-1]._centerSoul && c.m_currentStronghold >= Singleton<trainingMgr>::Instance().openStronghold(type))
        {
            m_soldier_souls[type-1].type = type;
            m_soldier_souls[type-1]._centerSoul = Singleton<trainingMgr>::Instance().getDefaultSoul(type,0);
            //兵魂
            for (int stype = 1; stype <= iSoulNum; ++stype)
            {
                m_soldier_souls[type-1]._soul[stype-1] = Singleton<trainingMgr>::Instance().getDefaultSoul(type,stype);
            }
            InsertSaveDb("insert into char_training (cid,sType,center_id,id_1,id_2,id_3,id_4,id_5) values (" + LEX_CAST_STR(_cid) +
                        "," + LEX_CAST_STR(type) + "," + LEX_CAST_STR(m_soldier_souls[type-1]._centerSoul->_id)
                        + "," + LEX_CAST_STR(m_soldier_souls[type-1]._soul[0]->_id)
                        + "," + LEX_CAST_STR(m_soldier_souls[type-1]._soul[1]->_id)
                        + "," + LEX_CAST_STR(m_soldier_souls[type-1]._soul[2]->_id)
                        + "," + LEX_CAST_STR(m_soldier_souls[type-1]._soul[3]->_id)
                        + "," + LEX_CAST_STR(m_soldier_souls[type-1]._soul[4]->_id)
                        + ")");
        }
    }
    return;
}

void CharTrainings::SaveSouls(int type)
{
    if (type < 1 || type > iTrainingNum)
    {
        for (int i = 0; i < iTrainingNum; ++i)
        {
            if (m_soldier_souls[i]._centerSoul)
            {
                std::string sqlcmd = "update char_training set center_id=" + LEX_CAST_STR(m_soldier_souls[i]._centerSoul->_id);
                for (int stype = 1; stype <= iSoulNum; ++stype)
                {
                    if (m_soldier_souls[i]._soul[stype-1])
                    {
                        sqlcmd += (",id_" + LEX_CAST_STR(stype) + "=" + LEX_CAST_STR(m_soldier_souls[i]._soul[stype-1]->_id));
                    }
                }
                sqlcmd += (" where cid=" + LEX_CAST_STR(_cid) + " and sType=" + LEX_CAST_STR(i+1));
                InsertSaveDb(sqlcmd);
            }
        }
    }
    else
    {
        if (m_soldier_souls[type-1]._centerSoul)
        {
            std::string sqlcmd = "update char_training set center_id=" + LEX_CAST_STR(m_soldier_souls[type-1]._centerSoul->_id);
            for (int stype = 1; stype <= iSoulNum; ++stype)
            {
                if (m_soldier_souls[type-1]._soul[stype-1])
                {
                    sqlcmd += (",id_" + LEX_CAST_STR(stype) + "=" + LEX_CAST_STR(m_soldier_souls[type-1]._soul[stype-1]->_id));
                }
            }
            sqlcmd += (" where cid=" + LEX_CAST_STR(_cid) + " and sType=" + LEX_CAST_STR(type));
            InsertSaveDb(sqlcmd);
        }
    }
}

//兵魂信息
void CharTrainings::getList(json_spirit::Array& slist)
{
    for (int i = 0; i < iTrainingNum; ++i)
    {
        if (m_soldier_souls[i]._centerSoul)
        {
            baseSoul* p_center = m_soldier_souls[i]._centerSoul;
            json_spirit::Object obj;
            json_spirit::Array list;
            for (int j = 0; j < iSoulNum; ++j)
            {
                if (m_soldier_souls[i]._soul[j])
                {
                    baseSoul* p = m_soldier_souls[i]._soul[j];
                    json_spirit::Object o;
                    o.push_back( Pair("type", p->_type) );
                    o.push_back( Pair("stype", p->_stype) );
                    o.push_back( Pair("level", p->_level) );
                    o.push_back( Pair("cost_type", p->_baseCost) );
                    o.push_back( Pair("cost_cnt", p->_costCount) );
                    if (p->_level > 0)
                    {
                        json_spirit::Object cur_add;
                        cur_add.push_back( Pair("moreDamage", p->_moreDamage) );
                        cur_add.push_back( Pair("bingli", p->_bingli) );
                        cur_add.push_back( Pair("attack", p->_attack) );
                        cur_add.push_back( Pair("wufang", p->_wufang) );
                        cur_add.push_back( Pair("cefang", p->_cefang) );
                        o.push_back( Pair("cur_add", cur_add) );
                    }
                    if (p->_next)
                    {
                        json_spirit::Object next_add;
                        next_add.push_back( Pair("moreDamage", p->_next->_moreDamage) );
                        next_add.push_back( Pair("bingli", p->_next->_bingli) );
                        next_add.push_back( Pair("attack", p->_next->_attack) );
                        next_add.push_back( Pair("wufang", p->_next->_wufang) );
                        next_add.push_back( Pair("cefang", p->_next->_cefang) );
                        o.push_back( Pair("next_add", next_add) );
                    }
                    list.push_back(o);
                }
            }
            obj.push_back( Pair("type", p_center->_type) );
            obj.push_back( Pair("stype", p_center->_stype) );
            obj.push_back( Pair("level", p_center->_level) );
            obj.push_back( Pair("list", list) );
            if (p_center->_level > 0)
            {
                json_spirit::Object cur_add;
                cur_add.push_back( Pair("moreDamage", p_center->_moreDamage) );
                cur_add.push_back( Pair("bingli", p_center->_bingli) );
                cur_add.push_back( Pair("attack", p_center->_attack) );
                cur_add.push_back( Pair("wufang", p_center->_wufang) );
                cur_add.push_back( Pair("cefang", p_center->_cefang) );
                obj.push_back( Pair("cur_add", cur_add) );
            }
            if (p_center->_next)
            {
                json_spirit::Object next_add;
                next_add.push_back( Pair("moreDamage", p_center->_next->_moreDamage) );
                next_add.push_back( Pair("bingli", p_center->_next->_bingli) );
                next_add.push_back( Pair("attack", p_center->_next->_attack) );
                next_add.push_back( Pair("wufang", p_center->_next->_wufang) );
                next_add.push_back( Pair("cefang", p_center->_next->_cefang) );
                obj.push_back( Pair("next_add", next_add) );
            }
            slist.push_back(obj);
        }
        else
        {
            json_spirit::Object obj;
            obj.push_back( Pair("type", i+1) );
            obj.push_back( Pair("needLevel", Singleton<trainingMgr>::Instance().openLevel(i+1)) );
            slist.push_back(obj);
        }
    }
}

//升级兵魂
int CharTrainings::levelUp(int type, int stype, json_spirit::Object& robj)
{
    if (type < 1 || type > iTrainingNum)
    {
        return HC_ERROR;
    }
    if (stype < 1 || stype > iSoulNum)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(_cid);
    if (!cdata.get())
        return HC_ERROR;
    if (m_soldier_souls[type - 1]._soul[stype - 1] && m_soldier_souls[type - 1]._soul[stype - 1]->_next)
    {
        int tr_counts = cdata->treasureCount(m_soldier_souls[type - 1]._soul[stype - 1]->_baseCost);
        if (tr_counts < m_soldier_souls[type - 1]._soul[stype - 1]->_costCount)
        {
            return HC_ERROR;
        }
        //扣除道具
        cdata->addTreasure(m_soldier_souls[type - 1]._soul[stype - 1]->_baseCost, -m_soldier_souls[type - 1]._soul[stype - 1]->_costCount);
#ifdef QQ_PLAT
        treasure_cost_tencent(cdata.get(),m_soldier_souls[type - 1]._soul[stype - 1]->_baseCost,m_soldier_souls[type - 1]._soul[stype - 1]->_costCount);
#endif
        m_soldier_souls[type - 1]._soul[stype - 1] = m_soldier_souls[type - 1]._soul[stype - 1]->_next;
        //act统计
        act_to_tencent(cdata.get(),act_new_soul_up,type,stype,m_soldier_souls[type - 1]._soul[stype - 1]->_level);

        //支线任务
        cdata->m_trunk_tasks.updateTask(task_upgrade_soul, type, stype);
        
        //是否需要升级魂眼
        bool eye_update = false;
        while (m_soldier_souls[type-1]._centerSoul && m_soldier_souls[type-1]._centerSoul->_level < minlevel(type))
        {
            if (m_soldier_souls[type-1]._centerSoul->_next)
            {
                m_soldier_souls[type-1]._centerSoul = m_soldier_souls[type-1]._centerSoul->_next;
                eye_update = true;

                //支线任务
                cdata->m_trunk_tasks.updateTask(task_center_soul_level, type, m_soldier_souls[type-1]._centerSoul->_level);
            }
            else
            {
                break;
            }
        }
        robj.push_back( Pair("eye_update", eye_update) );
        //升级完毕更新加成状态
        updateAttribute(*(cdata.get()));
        SaveSouls(type);
        //攻擊力變化
        cdata->set_attack_change();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int CharTrainings::minlevel(int type)
{
    int minlevel = 100;
    if (type < 1 || type > iTrainingNum)
    {
        return 0;
    }
    for (int stype = 1; stype <= iSoulNum; ++stype)
    {
        if (m_soldier_souls[type - 1]._soul[stype - 1] && m_soldier_souls[type - 1]._soul[stype - 1]->_level < minlevel)
        {
            minlevel = m_soldier_souls[type - 1]._soul[stype - 1]->_level;
        }
    }
    return minlevel;
}

//战斗属性
void CharTrainings::updateAttribute(CharData& cdata)
{
    _combatAttr.clear();
    int tmp_level = 0;
    for (int i = 0; i < iTrainingNum; ++i)
    {
        if (m_soldier_souls[i]._centerSoul)
        {
            baseSoul* p_center = m_soldier_souls[i]._centerSoul;
            if (p_center->_level > 0)
            {
                if (p_center->_bingli > 0)
                {
                    _combatAttr.m_hp_stype[0] += p_center->_bingli;
                    _combatAttr.m_hp_stype[1] += p_center->_bingli;
                    _combatAttr.m_hp_stype[2] += p_center->_bingli;
                    _combatAttr.m_hp_stype[3] += p_center->_bingli;
                    _combatAttr.m_hp_stype[4] += p_center->_bingli;
                }
                if (p_center->_attack > 0)
                {
                    _combatAttr.m_attack_stype[0] += p_center->_attack;
                    _combatAttr.m_attack_stype[1] += p_center->_attack;
                    _combatAttr.m_attack_stype[2] += p_center->_attack;
                    _combatAttr.m_attack_stype[3] += p_center->_attack;
                    _combatAttr.m_attack_stype[4] += p_center->_attack;
                }
                if (p_center->_wufang > 0)
                {
                    _combatAttr.m_wufang_stype[0] += p_center->_wufang;
                    _combatAttr.m_wufang_stype[1] += p_center->_wufang;
                    _combatAttr.m_wufang_stype[2] += p_center->_wufang;
                    _combatAttr.m_wufang_stype[3] += p_center->_wufang;
                    _combatAttr.m_wufang_stype[4] += p_center->_wufang;
                }
                if (p_center->_cefang > 0)
                {
                    _combatAttr.m_cefang_stype[0] += p_center->_cefang;
                    _combatAttr.m_cefang_stype[1] += p_center->_cefang;
                    _combatAttr.m_cefang_stype[2] += p_center->_cefang;
                    _combatAttr.m_cefang_stype[3] += p_center->_cefang;
                    _combatAttr.m_cefang_stype[4] += p_center->_cefang;
                }
            }
            //soul1-5对应步器策骑弓
            //stype(1步2弓3士4马5车)
            int tmp[5] = {1,5,3,4,2};
            for (int j = 0; j < iSoulNum; ++j)
            {
                if (m_soldier_souls[i]._soul[j])
                {
                    baseSoul* p = m_soldier_souls[i]._soul[j];
                    if (p->_level > 0)
                    {
                        tmp_level += p->_level;
                        int stype = tmp[j];
                        if (p->_moreDamage > 0)
                        {
                            _combatAttr.m_moredamage_stype[stype-1] += p->_moreDamage;
                        }
                        if (p->_bingli > 0)
                        {
                            _combatAttr.m_hp_stype[stype-1] += p->_bingli;
                        }
                        if (p->_attack > 0)
                        {
                            _combatAttr.m_attack_stype[stype-1] += p->_attack;
                        }
                        if (p->_wufang > 0)
                        {
                            _combatAttr.m_wufang_stype[stype-1] += p->_wufang;
                        }
                        if (p->_cefang > 0)
                        {
                            _combatAttr.m_cefang_stype[stype-1] += p->_cefang;
                        }
                    }
                }
            }
        }
    }
    _combatAttr.enable();

    _score = 0;
    if (cdata.m_level < 50)
    {
        _score = (int)((double)tmp_level * 100.0 / 150.0);
    }
    else if(cdata.m_level < 70)
    {
        _score = (int)((double)tmp_level * 100.0 / 600.0);
    }
    else
    {
        _score = (int)((double)tmp_level * 100.0 / 1500.0);
    }
    _score = _score > 100 ? 100 : _score;
    #if 0
    //travel soul_add
    for (int i = 0; i < iTrainingNum; ++i)
    {
        if (m_soldier_souls[i]._centerSoul)
        {
            cout << "*****************cid=" << _cid << "****SOULS****" << endl;
            baseSoul* p_center = m_soldier_souls[i]._centerSoul;
            cout << "type="<<i+1<<",center_id=" << p_center->_id << ",level=" << p_center->_level << endl;
            if (p_center->_level > 0)
            {
                cout << "bingli=" << p_center->_bingli;
                cout << " attack=" << p_center->_attack;
                cout << " wufang=" << p_center->_wufang;
                cout << " cefang=" << p_center->_cefang;
                cout << endl;
            }
            //soul1-5对应步器策骑弓
            //stype(1步2弓3士4马5车)
            int tmp[5] = {1,5,3,4,2};
            for (int j = 0; j < iSoulNum; ++j)
            {
                if (m_soldier_souls[i]._soul[j])
                {
                    baseSoul* p = m_soldier_souls[i]._soul[j];
                    cout << "type="<<i+1<<",stype="<<j+1<<",soul_id=" << p->_id << ",level=" << p->_level << endl;
                    if (p->_level > 0)
                    {
                        int stype = tmp[j];
                        cout << "**********this add true_stype=" << stype <<endl;
                        cout << "bingli=" << p->_bingli;
                        cout << " attack=" << p->_attack;
                        cout << " wufang=" << p->_wufang;
                        cout << " cefang=" << p->_cefang;
                        cout << " moreDamage=" << p->_moreDamage;
                        cout << endl;
                    }
                }
            }
        }
    }
    #endif
    return;
}

void CharTrainings::DaojuBack()
{
    boost::shared_ptr<CharData> cdata = GeneralDataMgr::getInstance()->GetCharData(_cid);
    if (!cdata.get())
        return;
    for (int i = 0; i < iTrainingNum; ++i)
    {
        if (m_soldier_souls[i]._centerSoul)
        {
            int t_type = 0, t_count = 0;
            //根据类型决定升级材料
            switch (m_soldier_souls[i]._centerSoul->_type)
            {
                case 1:
                    t_type = treasure_type_soul_type1;
                    break;
                case 2:
                    t_type = treasure_type_soul_type2;
                    break;
                case 3:
                    t_type = treasure_type_soul_type3;
                    break;
            }
            for (int j = 0; j < iSoulNum; ++j)
            {
                if (m_soldier_souls[i]._soul[j])
                {
                    baseSoul* p = m_soldier_souls[i]._soul[j];
                    if (p->_level > 0)
                    {
                        //合计返还数量
                        baseSoul* tmp = Singleton<trainingMgr>::Instance().getDefaultSoul(i+1,j+1);
                        while (tmp->_level < p->_level)
                        {
                            t_count += tmp->_costCount;
                            if (tmp->_next)
                            {
                                tmp = tmp->_next;
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                    m_soldier_souls[i]._soul[j] = Singleton<trainingMgr>::Instance().getDefaultSoul(i+1,j+1);
                }
            }
            m_soldier_souls[i]._centerSoul = Singleton<trainingMgr>::Instance().getDefaultSoul(i+1,0);
            SaveSouls(i+1);
            //发送信件
            if (t_type > 0 && t_count > 0)
            {
                json_spirit::Object getobj;
                getobj.push_back( Pair("type", item_type_treasure) );
                getobj.push_back( Pair("id", t_type) );
                getobj.push_back( Pair("count", t_count) );
                //通知被请求玩家
                sendSystemMail(cdata->m_name, cdata->m_id, strTrainingBackTitle, strTrainingBackContent, "["+json_spirit::write(getobj)+"]");
            }
        }
    }
    //升级完毕更新加成状态
    updateAttribute(*(cdata.get()));
    //攻擊力變化
    cdata->set_attack_change();
}

trainingMgr::trainingMgr()
{
    reload();
}

int trainingMgr::reload()
{
    m_max_soul_id = 0;
    Query q(GetDb());
    q.get_result("select id,type,stype,level,moreDamage,attack,wufang,cefang,bingli,up_count from base_souls where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        if (id > 10000 || id < 1)
        {
            ERR();
            continue;
        }
        _base_souls[id-1]._id = id;
        _base_souls[id-1]._type = q.getval();
        _base_souls[id-1]._stype = q.getval();
        _base_souls[id-1]._level = q.getval();
        _base_souls[id-1]._moreDamage = q.getval();
        _base_souls[id-1]._attack = q.getval();
        _base_souls[id-1]._wufang = q.getval();
        _base_souls[id-1]._cefang = q.getval();
        _base_souls[id-1]._bingli = q.getval();
        _base_souls[id-1]._baseCost = 0;
        //根据类型决定升级材料
        switch (_base_souls[id-1]._type)
        {
            case 1:
                _base_souls[id-1]._baseCost = treasure_type_soul_type1;
                break;
            case 2:
                _base_souls[id-1]._baseCost = treasure_type_soul_type2;
                break;
            case 3:
                _base_souls[id-1]._baseCost = treasure_type_soul_type3;
                break;
        }
        _base_souls[id-1]._costCount = q.getval();
        m_max_soul_id = id;
    }
    q.free_result();
    for (int i = 1; i <= m_max_soul_id; ++i)
    {
        baseSoul* next = getSoul(i+18);
        if (next && next->_type == _base_souls[i-1]._type && next->_stype == _base_souls[i-1]._stype)
        {
            _base_souls[i-1]._next = next;
        }
        else
        {
            _base_souls[i-1]._next = NULL;
        }
    }
    return HC_SUCCESS;
}

boost::shared_ptr<CharTrainings> trainingMgr::getChar(CharData& c)
{
    std::map<int, boost::shared_ptr<CharTrainings> >::iterator it = m_char_datas.find(c.m_id);
    if (it != m_char_datas.end() && it->second.get())
    {
        return it->second;
    }
    boost::shared_ptr<CharTrainings> r(new CharTrainings(c.m_id));
    m_char_datas[c.m_id] = r;
    r->load(c);
    return r;
}

baseSoul* trainingMgr::getSoul(int id)
{
    if (id >= 1 && id <= m_max_soul_id)
    {
        return _base_souls + id - 1;
    }
    return NULL;
}

baseSoul* trainingMgr::getDefaultSoul(int type, int stype)
{
    if (type >= 0 && type <= iTrainingNum && stype >= 0 && stype <= iSoulNum)
    {
        int id = (type-1) * (iSoulNum+1) + stype + 1;
        return getSoul(id);
    }
    return NULL;
}

int trainingMgr::openLevel()
{
    return openLevel(1);
}

int trainingMgr::openLevel(int type)
{
    if (type >= 0 && type <= iTrainingNum)
        return iTrainingOpenLevel[type-1];
    return 0;
}

int trainingMgr::openStronghold()
{
    return openStronghold(1);
}

int trainingMgr::openStronghold(int type)
{
    if (type >= 0 && type <= iTrainingNum)
        return iTrainingOpenStrongholdid[type-1];
    return 0;
}

int trainingMgr::getSoulsDaojuCost(int times)
{
    #if 0
    int need_gold = 2;
    int tmp = times / 5;
    tmp += (times % 5 > 0 ? 1 : 0);
    for (int i = 1; i < tmp; ++i)
    {
        need_gold += 2;
    }
    need_gold = (need_gold > 20 ? 20 : need_gold);
    return need_gold;
    #endif
    if (times < 1)
        return 0;
    if (times <= 10)
    {
        return times * 2;
    }
    else if(times <= 30)
    {
        return 20;
    }
    else if(times <= 50)
    {
        return 40;
    }
    else if(times <= 100)
    {
        return 80;
    }
    else if(times <= 300)
    {
        return 200;
    }
    else
    {
        return 400;
    }
    return 0;
}

void trainingMgr::SoulsDaojuBack(int cid)
{
    //获取处理玩家列表
    std::list<int> cid_list;
    if (cid == 0)
    {
        Query q(GetDb());
        //加载练兵
        q.get_result("select distinct(cid) from char_training where 1");
        CHECK_DB_ERR(q);
        while (q.fetch_row())
        {
            int id = q.getval();
            cid_list.push_back(id);
        }
        q.free_result();
    }
    else
    {
        cid_list.push_back(cid);
    }
    //处理玩家数据
    for (std::list<int>::iterator it = cid_list.begin(); it != cid_list.end(); ++it)
    {
        boost::shared_ptr<CharData> pc = GeneralDataMgr::getInstance()->GetCharData(*it);
        if (pc.get())
        {
            boost::shared_ptr<CharTrainings> ct = getChar(*pc);
            if (ct.get())
            {
                ct->DaojuBack();
            }
        }
    }
}

//查询练兵列表 
int ProcessQuerySoulList(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    if (pc->m_soulOpen == 0)
    {
        robj.push_back( Pair("open", 0) );
        return HC_SUCCESS;
    }
    robj.push_back( Pair("open", 1) );
    
    boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*pc);
    if (ct.get())
    {
        json_spirit::Array slist;
        ct->getList(slist);
        robj.push_back( Pair("list", slist) );
    }
    return HC_SUCCESS;
}

//升级练兵兵魂
int ProcessUpgradeSoul(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    int type = 0, stype = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    READ_INT_FROM_MOBJ(stype, o, "stype");
    robj.push_back( Pair("type", type) );
    robj.push_back( Pair("stype", stype) );
    CharData* pc = m_ponlineuser->m_onlineCharactor->m_charactor.get();
    if (pc->m_soulOpen == 0)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<CharTrainings> ct = Singleton<trainingMgr>::Instance().getChar(*pc);
    if (ct.get())
    {
        return ct->levelUp(type,stype, robj);
    }
    return HC_ERROR;
}

//购买界面
int ProcessGetSoulsCostInfo(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* pc = NULL;
    int ret = getSessionChar(psession, pc);
    if (ret != HC_SUCCESS || NULL == pc)
    {
        return ret;
    }
    json_spirit::Array list;
    json_spirit::Object info;
    int needgold = 0, cnt = 10;
    int dj_count = pc->m_bag.getGemCount(treasure_type_soul_ling);
    int already_time = pc->queryExtraData(char_data_type_daily, char_data_daily_buy_souls_daoju1);
    info.push_back( Pair("tid", treasure_type_soul_type1));
    info.push_back( Pair("gold", Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + 1)));
    for (int i = dj_count + 1; i <= cnt; ++i)
    {
        needgold += Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + i);
    }
    info.push_back( Pair("massGold1", needgold));
    list.push_back(info);

    info.clear();
    needgold = 0;
    already_time = pc->queryExtraData(char_data_type_daily, char_data_daily_buy_souls_daoju2);
    info.push_back( Pair("tid", treasure_type_soul_type2));
    info.push_back( Pair("gold", Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + 1)));
    for (int i = dj_count + 1; i <= cnt; ++i)
    {
        needgold += Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + i);
    }
    info.push_back( Pair("massGold1", needgold));
    list.push_back(info);
    
    info.clear();
    needgold = 0;
    already_time = pc->queryExtraData(char_data_type_daily, char_data_daily_buy_souls_daoju3);
    info.push_back( Pair("tid", treasure_type_soul_type3));
    info.push_back( Pair("gold", Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + 1)));
    for (int i = dj_count + 1; i <= cnt; ++i)
    {
        needgold += Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + i);
    }
    info.push_back( Pair("massGold1", needgold));
    list.push_back(info);
    
    robj.push_back( Pair("list", list));
    robj.push_back( Pair("soul_ling", dj_count) );
    return HC_SUCCESS;
}

//购买兵魂碎片
int ProcessBuySoulsDaoju(session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
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
    int tid = 0, type = 1, extra_data_type;
    READ_INT_FROM_MOBJ(tid,o,"id");
    READ_INT_FROM_MOBJ(type,o,"type");
    robj.push_back( Pair("id", tid) );
    robj.push_back( Pair("type", type) );
    switch(tid)
    {
        case treasure_type_soul_type1:
            extra_data_type = char_data_daily_buy_souls_daoju1;
            break;
        case treasure_type_soul_type2:
            extra_data_type = char_data_daily_buy_souls_daoju2;
            break;
        case treasure_type_soul_type3:
            extra_data_type = char_data_daily_buy_souls_daoju3;
            break;
        default:
            return HC_ERROR;
            break;
    }
    boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(tid);
    if (!bt.get())
    {
        return HC_ERROR;
    }
    int cnt = 0;
    bool be_double = false;
    if (type == 1)
    {
        cnt = 1;
    }
    else if(type == 2)
    {
        cnt = 10;
    }
    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }

    int dj_count = pc->m_bag.getGemCount(treasure_type_soul_ling);
    int dj_use = 0;
    if (bt->max_size > 0)
    {
        int left = pc->m_bag.size() - pc->m_bag.getUsed();
        left *= bt->max_size;
        if (left < cnt)
        {
            cnt = left;
        }
    }
    int already_time = pc->queryExtraData(char_data_type_daily, extra_data_type);
    if (cnt > 0)
    {
        int needgold = 0, more_cnt = 0;
        for (int i = 1; i <= cnt; ++i)
        {
            if (dj_use < dj_count)
            {
                ++dj_use;
            }
            else
            {
                needgold += Singleton<trainingMgr>::Instance().getSoulsDaojuCost(already_time + i);
            }
        }
        if (-1 == pc->addGold(-needgold))
        {
            return HC_ERROR_NOT_ENOUGH_GOLD;
        }

        if (dj_use > 0)
        {
            pc->addTreasure(treasure_type_soul_ling, -dj_use);

            //通知道具消耗
            add_statistics_of_treasure_cost(pc->m_id,pc->m_ip_address,treasure_type_soul_ling,dj_use,treasure_unknow,2,pc->m_union_id,pc->m_server_id);
        }
        //金币消耗统计
        add_statistics_of_gold_cost(pc->m_id, pc->m_ip_address, needgold, gold_cost_for_treasure+tid, pc->m_union_id, pc->m_server_id);
#ifdef QQ_PLAT
        gold_cost_tencent(pc,needgold,gold_cost_for_buy_daoju, tid, cnt);
#endif
        pc->setExtraData(char_data_type_daily, extra_data_type, already_time+cnt);
        for (int i = 1; i <= cnt; ++i)
        {
            if (my_random(1,100) <= 20)
            {
                ++more_cnt;
                be_double = true;
            }
        }
        //给道具
        int err_code = 0;
        pc->m_bag.addGem(tid, cnt+more_cnt,err_code);
        pc->NotifyCharData();
        std::string msg = "";
        if (be_double)
        {
            msg = strBuyCri;
            robj.push_back( Pair("baoji", 1) );
        }
        else
        {
            msg = strBuySuc;
        }
        str_replace(msg, "$R", bt->name + strCounts + LEX_CAST_STR(cnt+more_cnt));

        //通知道具消耗
        if (dj_use > 0)
        {
            msg += "\n" + treasure_expend_msg(treasure_type_soul_ling, dj_use);
        }
        robj.push_back( Pair("msg", msg) );
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR;
    }
    return HC_ERROR;
}

