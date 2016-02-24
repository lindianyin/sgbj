
#include "char_jxl.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "singleton.h"
#include "net.h"
#include "statistics.h"

using namespace net;

Database& GetDb();
extern int getSessionChar(session_ptr& psession, CharData* &pc);
extern int getSessionChar(session_ptr& psession, boost::shared_ptr<CharData>& cdata);
void InsertSaveDb(const std::string& sql);

//返回  0未激活，1激活 2启用 3可招募，招募后可激活 4可招募
int getJxlState(CharData& cdata, jxl_combo& jxl)
{
    char_jxl_buff& cj = cdata.m_jxl_buff;
    for (int i = 0; i < iJxlBuffTotalType; ++i)
    {
        if (cj.used_buff[i].get() == &jxl)
        {
            return 2;
        }
    }

    bool canBuy = false, canNotBuy = false;
    for (std::list<int>::iterator it = jxl.glist.begin(); it != jxl.glist.end(); ++it)
    {
        if (false == cdata.HasGeneral(*it))
        {
            if (cdata.canBuyOfficalGeneral(*it))
            {
                canBuy = true;
            }
            else
            {
                canNotBuy = true;
            }
        }
    }
    if (canBuy && canNotBuy)
    {
        return 4;
    }
    else if (canBuy)
    {
        return 3;
    }
    else if (canNotBuy)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

//查询将星录
int ProcessQueryJxl(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (pc->m_jxlOpen)
    {
        return Singleton<jxl_mgr>::Instance().queryJxl(*pc, robj);
    }
    else
    {
        return HC_ERROR;
    }
}


//查询将星录详细
int ProcessQueryJxlDetail(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();
    if (pc->m_jxlOpen)
    {
        int id = 1;
        READ_INT_FROM_MOBJ(id,o,"id");
        return Singleton<jxl_mgr>::Instance().queryJxlDetail(id, *pc, robj);
    }
    else
    {
        return HC_ERROR;
    }
}

//启用将星录星符
int ProcessApplyJxl(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    CharData* pc = cdata.get();

    if (pc->m_jxlOpen == 0)
    {
        return HC_ERROR;
    }
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"id");
    boost::shared_ptr<jxl_combo> c = Singleton<jxl_mgr>::Instance().getCombo(id);
    if (c.get())
    {
        int state = getJxlState(*pc, *(c.get()));
        if (state == 1)
        {
            char_jxl_buff& cj = pc->m_jxl_buff;
            cj.used_buff[c->type-1] = c;
            cj.update();
            cj.save();
            robj.push_back( Pair("type", c->type) );
            robj.push_back( Pair("id", c->id) );
            //act统计
            act_to_tencent(pc,act_new_jxl,c->type);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

jxl_mgr::jxl_mgr()
{
    Query q(GetDb());
    q.get_result("select id,type,name,memo from base_jxl where 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        jxl_combo* c = new jxl_combo;
        c->id = q.getval();
        c->type = q.getval();
        c->name = q.getstr();
        c->memo = q.getstr();
        c->obj.push_back( Pair("id", c->id) );
        c->obj.push_back( Pair("type", c->type) );
        c->obj.push_back( Pair("name", c->name) );
        c->obj.push_back( Pair("memo", c->memo) );

        boost::shared_ptr<jxl_combo> sc(c);
        m_jxl_combo.push_back(sc);
        assert(m_jxl_combo.size() == c->id);
    }
    q.free_result();

    for (std::vector<boost::shared_ptr<jxl_combo> >::iterator it = m_jxl_combo.begin(); it != m_jxl_combo.end(); ++it)
    {
        if (it->get())
        {
            jxl_combo* c = it->get();
            q.get_result("select gid from base_jxl_generals where id=" + LEX_CAST_STR(c->id));
            CHECK_DB_ERR(q);
            while (q.fetch_row())
            {
                int gid = q.getval();
                officalgenerals* og = GeneralDataMgr::getInstance()->getOfficalGeneral(gid);
                if (og)
                {
                    if (og->need_offical)
                    {
                        m_jxl_officals[og->need_offical] = 1;
                    }
                    if (og->need_slevel)
                    {
                        m_jxl_stronghols[og->need_slevel] = 1;
                    }
                }
                c->glist.push_back(gid);
            }
            q.free_result();

            q.get_result("select hp,attack,fang,hp_p,attack_p,fang_p,hit,crit,shipo,parry,resist_hit,resist_crit,resist_shipo,resist_parry from base_jxl_attr where id=" + LEX_CAST_STR(c->id));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                c->attr.hp = q.getval();
                c->attr.attack = q.getval();
                c->attr.fang = q.getval();
                c->attr.hp_percent = q.getval();
                c->attr.attack_percent = q.getval();
                c->attr.fang_percent = q.getval();
                c->attr.hit = q.getval();
                c->attr.crit = q.getval();
                c->attr.shipo = q.getval();
                c->attr.parry = q.getval();
                c->attr.resist_hit = q.getval();
                c->attr.resist_crit = q.getval();
                c->attr.resist_shipo = q.getval();
                c->attr.resist_parry = q.getval();
            }
            q.free_result();

            //cout<<"id:"<<c->id<<"=>";
            //c->attr.dump();
        }
    }
}

bool jxl_mgr::needNotifyOffical(int offical)
{
    return m_jxl_officals.find(offical) != m_jxl_officals.end();
}

bool jxl_mgr::needNotifyStronghold(int stronghold)
{
    return m_jxl_stronghols.find(stronghold) != m_jxl_stronghols.end();
}

int jxl_mgr::queryJxl(CharData& cdata, json_spirit::Object& robj)
{
    json_spirit::Array list;
    for (std::vector<boost::shared_ptr<jxl_combo> >::iterator it = m_jxl_combo.begin(); it != m_jxl_combo.end(); ++it)
    {
        jxl_combo* p = it->get();
        if (p)
        {
            json_spirit::Object obj;
            obj.push_back( Pair("id", p->id) );
            obj.push_back( Pair("type", p->type) );
            obj.push_back( Pair("name", p->name) );
            obj.push_back( Pair("memo", p->memo) );
            int state = getJxlState(cdata, *p);
            obj.push_back( Pair("state", state) );
            list.push_back(obj);
        }
    }
    robj.push_back( Pair("list", list) );
    return HC_SUCCESS;
}

int jxl_mgr::queryJxlDetail(int id, CharData& cdata, json_spirit::Object& robj)
{
    boost::shared_ptr<jxl_combo> c = getCombo(id);
    if (c.get())
    {
        json_spirit::Array list;
        jxl_combo* pc = c.get();
        for (std::list<int>::iterator it = pc->glist.begin(); it != pc->glist.end(); ++it)
        {
            boost::shared_ptr<GeneralTypeData> g = GeneralDataMgr::getInstance()->GetBaseGeneral(*it);
            if (g.get())
            {
                json_spirit::Object obj;
                obj.push_back( Pair("name", g->m_name) );
                obj.push_back( Pair("jxl", g->m_jxl) );

                json_spirit::Object tianfu;
                tianfu.push_back( Pair("name", g->m_new_tianfu.m_name) );
                tianfu.push_back( Pair("memo", g->m_new_tianfu.m_memo) );                
                obj.push_back( Pair("tianfu", tianfu) );

                obj.push_back( Pair("spic", g->m_spic) );
                obj.push_back( Pair("color", g->m_quality) );
                if (g->m_speSkill.get())
                {
                    obj.push_back( Pair("attack_range", g->m_speSkill->attack_type));
                    obj.push_back( Pair("spe_name", g->m_speSkill->name));
                    obj.push_back( Pair("spe_memo", g->m_speSkill->memo));
                }
                if (cdata.HasGeneral(*it))
                {
                    obj.push_back( Pair("have", 1) );
                }
                else
                {
                    //可以招募
                    if (cdata.canBuyOfficalGeneral(*it))
                    {
                        obj.push_back( Pair("can", *it) );
                    }
                }
                list.push_back(obj);
            }
        }
        int state = getJxlState(cdata, *pc);
        robj.push_back( Pair("state", state) );
        robj.push_back( Pair("list", list) );
        robj.push_back( Pair("name", pc->name) );
        robj.push_back( Pair("memo", pc->memo) );
        robj.push_back( Pair("id", pc->id) );
        robj.push_back( Pair("type", pc->type) );
    }    
    return HC_SUCCESS;
}

void jxl_mgr::getAction(CharData& cdata, json_spirit::Array& elist)
{
    if (cdata.m_jxlOpen)
    {
        json_spirit::Object obj;
        obj.push_back( Pair("type", top_level_event_jxl) );
        int active = 0;
        for (std::vector<boost::shared_ptr<jxl_combo> >::iterator it = m_jxl_combo.begin(); it != m_jxl_combo.end(); ++it)
        {
            jxl_combo* p = it->get();
            if (p)
            {
                int state = getJxlState(cdata, *p);
                if (state == 3)
                {
                    active = 1;
                    break;
                }
            }
        }
        obj.push_back( Pair("active", active) );
        elist.push_back(obj);
    }
}

boost::shared_ptr<jxl_combo> jxl_mgr::getCombo(int id)
{
    if (id > 0 && id <= m_jxl_combo.size())
    {
        return m_jxl_combo[id-1];
    }    
    else
    {
        boost::shared_ptr<jxl_combo> c;
        return c;
    }
}

//招募了某个武将后，是否激活了新的将星录
int jxl_mgr::checkActivation(CharData& cdata, int gid)
{
    int ret = 0;
    bool need_check = false;
    for (std::vector<boost::shared_ptr<jxl_combo> >::iterator it = m_jxl_combo.begin(); it != m_jxl_combo.end(); ++it)
    {
        jxl_combo* p = it->get();
        if (p)
        {
            bool have_this = false;
            for (std::list<int>::iterator itg = p->glist.begin(); itg != p->glist.end(); ++itg)
            {
                if (*itg == gid)
                {
                    have_this = true;
                    need_check = true;
                    break;
                }
            }
            if (have_this)
            {
                int state = getJxlState(cdata, *p);
                if (state == 1)
                {
                    ret = p->id;
                    break;
                }
            }
        }
    }
    if (need_check)
    {
        int active = 0;
        for (std::vector<boost::shared_ptr<jxl_combo> >::iterator it = m_jxl_combo.begin(); it != m_jxl_combo.end(); ++it)
        {
            jxl_combo* p = it->get();
            if (p)
            {
                int state = getJxlState(cdata, *p);
                if (state == 3)
                {
                    active = 1;
                    break;
                }
            }
        }
        cdata.notifyEventState(top_level_event_jxl, active, 0);
    }
    return ret;
}

char_jxl_buff::char_jxl_buff(CharData& c)
:cdata(c)
{
    
}

void char_jxl_buff::load()
{
    Query q(GetDb());
    q.get_result("select buff1,buff2,buff3,buff4 from char_jxl_buffs where cid=" + LEX_CAST_STR(cdata.m_id));
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        for (int i = 0; i < 4; ++i)
        {
            int id = q.getval();
            used_buff[i] = Singleton<jxl_mgr>::Instance().getCombo(id);
        }
        q.free_result();
        update();
    }
    else
    {
        q.free_result();
        if (cdata.m_jxlOpen)
        {
            InsertSaveDb("insert into char_jxl_buffs (cid,buff1,buff2,buff3,buff4) values (" + LEX_CAST_STR(cdata.m_id)
                + ",0,0,0,0)");
        }
    }
}

void char_jxl_buff::update()
{
    total_buff_attr.clear();
    for (int i = 0; i < iJxlBuffTotalType; ++i)
    {
        jxl_combo* c = used_buff[i].get();
        if (c)
        {
            total_buff_attr.hp += c->attr.hp;
            total_buff_attr.attack += c->attr.attack;
            total_buff_attr.fang += c->attr.fang;
            total_buff_attr.hp_percent += c->attr.hp_percent;
            total_buff_attr.attack_percent += c->attr.attack_percent;
            total_buff_attr.fang_percent += c->attr.fang_percent;
            total_buff_attr.hit += c->attr.hit;
            total_buff_attr.crit += c->attr.crit;
            total_buff_attr.shipo += c->attr.shipo;
            total_buff_attr.parry += c->attr.parry;
            total_buff_attr.resist_hit += c->attr.resist_hit;
            total_buff_attr.resist_crit += c->attr.resist_crit;
            total_buff_attr.resist_shipo += c->attr.resist_shipo;
            total_buff_attr.resist_parry += c->attr.resist_parry;
        }
    }
    cdata.set_attack_change();
}

void char_jxl_buff::save()
{
    InsertSaveDb("update char_jxl_buffs set buff1=" + LEX_CAST_STR(used_buff[0].get() ? used_buff[0]->id : 0)
        + ",buff2=" + LEX_CAST_STR(used_buff[1].get() ? used_buff[1]->id : 0)
        + ",buff3=" + LEX_CAST_STR(used_buff[2].get() ? used_buff[2]->id : 0)
        + ",buff4=" + LEX_CAST_STR(used_buff[3].get() ? used_buff[3]->id : 0)
        + " where cid=" + LEX_CAST_STR(cdata.m_id));
}

void char_jxl_buff::getInfo(json_spirit::Array& list)
{
    list.clear();
    for (int i = 0; i < iJxlBuffTotalType; ++i)
    {
        if (used_buff[i].get())
        {
            list.push_back(used_buff[i]->obj);
        }
    }
}

//百分比增加的战力-输入为每个阵上武将的攻击，血量，普防，策防
int jxl_buff_attr::get_attack(int attack_, int hp_, int wufang_, int cefang_)
{
    attack_ += attack;
    hp_ += hp;
    wufang_ += fang;
    cefang_ += fang;

    attack_ = attack_ * (attack_percent)/100 + attack;
    hp_ = hp_ * (hp_percent)/100 + hp;
    wufang_ = wufang_ * (fang_percent)/100 + fang;
    cefang_ = cefang_ * (fang_percent)/100 + fang;
    int total = (attack_ * 2 + wufang_ + cefang_ + hp_);
    int special = hit * hit + crit * crit + shipo * shipo + parry*parry;
    special += (resist_hit*resist_hit + resist_crit*resist_crit);
    special += (resist_shipo*resist_shipo + resist_parry*resist_parry);
    return total + special/100;
}

//百分比增加的战力-输入为每个阵上武将的攻击，血量，普防，策防
void jxl_buff_attr::get_add(int attack_, int hp_,
                            int wufang_, int cefang_,
                            int& attack_add, int& hp_add,
                            int& wufang_add, int& cefang_add)
{
    attack_add = attack + (attack_+attack)*attack_percent/100;
    hp_add = hp + (hp_+hp)*hp_percent/100;
    wufang_add = fang + (wufang_+fang)*fang_percent/100;
    cefang_add = fang + (cefang_+fang)*fang_percent/100;    
}

//增加特效
void jxl_buff_attr::add_special(combatAttribute& cb)
{
    cb.add_resist_level(special_attack_dodge, hit);
    cb.add_special_attack_level(special_attack_baoji, crit);
    cb.add_special_attack_level(special_attack_shipo, shipo);
    cb.add_special_attack_level(special_attack_parry, parry);
    cb.add_special_attack_level(special_attack_dodge, resist_hit);
    cb.add_resist_level(special_attack_baoji, resist_crit);
    cb.add_resist_level(special_attack_shipo, resist_shipo);
    cb.add_resist_level(special_attack_parry, resist_parry);
}

void jxl_buff_attr::dump()
{
    cout<<"hp:"<<hp<<",attack:"<<attack<<",fang:"<<fang<<",hp_p:"<<hp_percent<<",attack_p:"<<attack_percent<<",fang_p:"<<fang_percent;
    cout<<"hit:"<<hit<<",crit:"<<crit<<",ship:"<<shipo<<",parry:"<<parry<<",resist_hit:"<<resist_hit<<",resist_crit:"<<resist_crit;
    cout<<"resist_shipo:"<<resist_shipo<<endl;
}
