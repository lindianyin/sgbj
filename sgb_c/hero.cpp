
#include "hero.h"
#include "ThreadLocalSingleton.h"
#include <boost/random/discrete_distribution.hpp>
#include "errcode_def.h"
#include "utils_all.h"
#include "data.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "SaveDb.h"
#include "spls_timer.h"
#include "city.h"
#include "action.h"
#include "utils_lang.h"
#include "pk.h"
#include "relation.h"

#define INFO(x) cout<<x

Database& GetDb();
void InsertSaveDb(const std::string& sql);
void InsertSaveDb(const saveDbJob& job);

extern std::string strEpicHeroGet;

//查询英雄信息
int ProcessGetHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    //获得英雄数据
    int hid = 0, purpose = 0;
    READ_INT_FROM_MOBJ(hid, o, "id");
    READ_INT_FROM_MOBJ(purpose, o, "purpose");
    json_spirit::Object hero_obj;
    if (purpose == 1)//查询基础英雄
    {
        boost::shared_ptr<baseHeroData> base_hd = Singleton<HeroMgr>::Instance().GetBaseHero(hid);
        if (!base_hd.get())
        {
            return HC_ERROR;
        }
        int star = 1, level = 1;
        READ_INT_FROM_MOBJ(star, o, "star");
        READ_INT_FROM_MOBJ(level, o, "level");
        hero_obj.push_back( Pair("id", hid));
        hero_obj.push_back( Pair("star", star));
        hero_obj.push_back( Pair("level", level));
        hero_obj.push_back( Pair("exp", 0));
        hero_obj.push_back( Pair("quality", base_hd->m_quality));
        if (level > 0)
        {
            double fac_a = 0.0, fac_b = 0.0, fac_c = 0.0, fac_d = 0.0;
            int att = 0, def = 0, magic = 0, hp = 0;
			READ_REAL_FROM_MOBJ(fac_a,o,"fac_a");
			READ_REAL_FROM_MOBJ(fac_b,o,"fac_b");
			READ_REAL_FROM_MOBJ(fac_c,o,"fac_c");
			READ_REAL_FROM_MOBJ(fac_d,o,"fac_d");
            hero_obj.push_back( Pair("attack_add", fac_a));
            hero_obj.push_back( Pair("defense_add", fac_b));
            hero_obj.push_back( Pair("magic_add", fac_c));
            hero_obj.push_back( Pair("hp_add", fac_d));
            att = base_hd->m_base_attack + fac_a * (level - 1);
            def = base_hd->m_base_defense + fac_b * (level - 1);
            magic = base_hd->m_base_magic + fac_c * (level - 1);
            hp = base_hd->m_base_hp + fac_d * (level - 1);
            //总战斗属性
            hero_obj.push_back( Pair("attack", att));
            hero_obj.push_back( Pair("defense", def));
            hero_obj.push_back( Pair("magic", magic));
            hero_obj.push_back( Pair("hp", hp));
            //英雄自身战斗属性
            hero_obj.push_back( Pair("hero_attack", att));
            hero_obj.push_back( Pair("hero_defense", def));
            hero_obj.push_back( Pair("hero_magic", magic));
            hero_obj.push_back( Pair("hero_hp", hp));
        }
        base_hd->toObj(hero_obj);
    }
    else if(purpose == 2)//查询玩家具体英雄
    {
        int cid = GeneralDataMgr::getInstance()->getHeroOwner(hid);
        if (cid)
        {
            CharData* pcc = GeneralDataMgr::getInstance()->GetCharData(cid).get();
            if (pcc)
            {
                std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = pcc->m_heros.m_heros.find(hid);
                if (it != pcc->m_heros.m_heros.end() && it->second.get())
                {
                    it->second->toObj(hero_obj);
                }
            }
        }
        else
        {
            return HC_ERROR;
        }
    }
    else if(purpose == 3)
    {
        ;//3用作服务端通知
    }
    if (purpose == 4)//查询基础神将英雄
    {
        boost::shared_ptr<baseHeroData> base_hd = Singleton<HeroMgr>::Instance().GetBaseHero(hid);
        if (!base_hd.get() || base_hd->m_epic == 0)
        {
            return HC_ERROR;
        }
        base_hd->toEpicObj(hero_obj);
    }
    robj.push_back( Pair("info", hero_obj));
    robj.push_back( Pair("purpose", purpose));
    return HC_SUCCESS;
}

//显示角色英雄列表
int ProcessCharHeros(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    //窗口展现类型，仅帮客户端储存返回，不处理
    int type = 0;
    READ_INT_FROM_MOBJ(type, o, "type");
    robj.push_back( Pair("type", type));
    //页面信息
    int page = 1, nums_per_page = 0;
    READ_INT_FROM_MOBJ(page, o, "page");
    READ_INT_FROM_MOBJ(nums_per_page, o, "pageNums");
    //筛选条件
    int race = 0, quality = 0, rank = 0, level = 0, epic = 0;
    READ_INT_FROM_MOBJ(race, o, "race");
    READ_INT_FROM_MOBJ(quality, o, "quality");
    READ_INT_FROM_MOBJ(rank, o, "rank");
    READ_INT_FROM_MOBJ(level, o, "level");
    READ_INT_FROM_MOBJ(epic, o, "epic");
    robj.push_back( Pair("race", race));
    robj.push_back( Pair("quality", quality));
    robj.push_back( Pair("rank", rank));
    robj.push_back( Pair("level", level));
    robj.push_back( Pair("epic", epic));
    if (page <= 0)
    {
        page = 1;
    }
    if (nums_per_page <= 0)
    {
        nums_per_page = 10;
    }
    //筛选出英雄
    std::list<boost::shared_ptr<CharHeroData> > char_heros_list;
    char_heros_list.clear();
    std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = cdata->m_heros.m_heros.begin();
    while (it != cdata->m_heros.m_heros.end())
    {
        if (!it->second.get() || !it->second->m_baseHero.get())
        {
            ++it;
            continue;
        }
        //有种族筛选
        if (race != 0 && it->second->m_race != race)
        {
            ++it;
            continue;
        }
        //有品质筛选
        if (quality != 0 && it->second->m_quality != quality)
        {
            ++it;
            continue;
        }
        //有牌面筛选
        if (rank != 0 && getLevelRank(it->second->m_level) != rank)
        {
            ++it;
            continue;
        }
        //有等级筛选
        if (level != 0 && it->second->m_level != level)
        {
            ++it;
            continue;
        }
        //有神将筛选
        if (epic == 0 && it->second->m_baseHero->m_epic > 0)
        {
            ++it;
            continue;
        }
        std::list<boost::shared_ptr<CharHeroData> >::iterator it_c = char_heros_list.begin();
        while (it_c != char_heros_list.end())
        {
            if (!(*it_c).get())
            {
                ++it_c;
                continue;
            }
            if ((*it_c)->m_quality < it->second->m_quality)
            {
                break;
            }
            else if((*it_c)->m_quality == it->second->m_quality && (*it_c)->m_level < it->second->m_level)
            {
                break;
            }
            ++it_c;
        }
        char_heros_list.insert(it_c,it->second);
        ++it;
    }
    //分页显示
    int cur_nums = 0;
    json_spirit::Array hero_array;
    if (char_heros_list.size())
    {
        //指定英雄定位页码
        int heroid = 0, heropos = 0;
        std::string heroname = "";
        READ_INT_FROM_MOBJ(heroid, o, "id");
        READ_STR_FROM_MOBJ(heroname, o, "hname");
        if (heroid != 0 || heroname != "")
        {
            std::list<boost::shared_ptr<CharHeroData> >::iterator itt = char_heros_list.begin();
            while (itt != char_heros_list.end())
            {
                if (!(*itt).get() || !(*itt)->m_baseHero.get())
                {
                    ++itt;
                    continue;
                }
                ++heropos;
                if ((*itt)->m_id == heroid || (*itt)->m_baseHero->m_name == heroname)
                {
                    break;
                }
                ++itt;
            }
            if (heropos != 0)
            {
                page = heropos / nums_per_page;
                if (heropos % nums_per_page > 0)
                    ++page;
            }
        }
        //根据边界显示分页
        int first_nums = nums_per_page * (page - 1)+ 1;
        int last_nums = nums_per_page * page;
        std::list<boost::shared_ptr<CharHeroData> >::iterator itt = char_heros_list.begin();
        while (itt != char_heros_list.end())
        {
            if (!(*itt).get())
            {
                ++itt;
                continue;
            }
            ++cur_nums;
            if (cur_nums >= first_nums && cur_nums <= last_nums)
            {
                boost::shared_ptr<CharHeroData> gd = (*itt);
                if (gd.get())
                {
                    json_spirit::Object hero_obj;
                    gd->toObj(hero_obj);
                    hero_array.push_back(hero_obj);
                }
                else
                {
                    ERR();
                }
            }
            ++itt;
        }
    }

    robj.push_back( Pair("list", hero_array));
    int maxpage = cur_nums/nums_per_page + ((cur_nums%nums_per_page) ? 1 : 0);
    if (maxpage == 0)
    {
        maxpage = 1;
    }
    json_spirit::Object pageobj;
    pageobj.push_back( Pair("maxPage", maxpage) );
    pageobj.push_back( Pair("page", page) );
    pageobj.push_back( Pair("pageNums", nums_per_page) );
    robj.push_back( Pair("page", pageobj) );

    robj.push_back( Pair("usedSize", cdata->m_heros.m_heros.size()) );
    robj.push_back( Pair("heroSize", cdata->m_heros.m_hero_max) );
    robj.push_back( Pair("buyedSize", cdata->m_heros.m_hero_max - HERO_DEFAULT_SIZE) );
    return HC_SUCCESS;
}

//设置出战英雄
int ProcessSetDefaultHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int hid = 0;
    READ_INT_FROM_MOBJ(hid, o, "id");
    return cdata->m_heros.SetDefault(hid,robj);
}

//获取英雄包信息
int ProcessQueryHeroPack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    int type = 1;
    READ_INT_FROM_MOBJ(type,o,"type");
    boost::shared_ptr<base_hero_pack> pbhp = Singleton<HeroMgr>::Instance().GetBaseHeroPack(type);
    if (pbhp)
    {
        pbhp->toObj(robj);
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//开启英雄包
int ProcessOpenHeroPack(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<HeroMgr>::Instance().OpenHeroPack(robj,o,cdata);
}

//购买英雄位
int ProcessBuyHeroSize(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    CharData* cdata = NULL;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || NULL == cdata)
    {
        return ret;
    }
    int num = 0;
    READ_INT_FROM_MOBJ(num, o, "num");
    return cdata->m_heros.buyHeroSize(num, robj);
}

//合成概率信息
int ProcessCompoundHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.CompoundHeroInfo(robj,o);
}

//合成英雄
int ProcessCompoundHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.CompoundHero(robj,o);
}

//分解英雄信息
int ProcessDecomposeHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.DecomposeHeroInfo(robj,o);
}

//分解英雄
int ProcessDecomposeHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.DecomposeHero(o);
}

//熔炼英雄信息
int ProcessSmeltHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.SmeltHeroInfo(robj);
}

//刷新熔炼英雄
int ProcessSmeltHeroRefresh(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.SmeltHeroRefresh();
}

//熔炼英雄
int ProcessSmeltHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.SmeltHero(o, robj);
}

//点金信息
int ProcessGoldenHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.GoldenHeroInfo(robj,o);
}

//点金英雄
int ProcessGoldenHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.GoldenHero(robj,o);
}

//显示神将英雄
int ProcessEpicHeros(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    json_spirit::Array epic_array;
    std::vector<int> epic_list = Singleton<HeroMgr>::Instance().GetEpicList();
    if (epic_list.size())
    {
        for (size_t i = 0; i < epic_list.size(); ++i)
        {
            boost::shared_ptr<baseHeroData> base_hd = Singleton<HeroMgr>::Instance().GetBaseHero(epic_list[i]);
            if (!base_hd.get())
            {
                continue;
            }
            int cur_id = cdata->m_heros.GetHeroByType(epic_list[i]);
            if (cur_id)//有当前英雄
            {
                json_spirit::Object o;
                o.push_back( Pair("hid", base_hd->m_hid));
                o.push_back( Pair("name", base_hd->m_name));
                o.push_back( Pair("spic", base_hd->m_spic));
                o.push_back( Pair("star", base_hd->m_quality));
                o.push_back( Pair("quality", base_hd->m_quality));
                o.push_back( Pair("id", cur_id));
                epic_array.push_back(o);
            }
            else
            {
                int next_id = cdata->m_heros.GetHeroByType(base_hd->m_next_hid);
                if (next_id)//有进阶英雄
                {
                    boost::shared_ptr<baseHeroData> next_hd = Singleton<HeroMgr>::Instance().GetBaseHero(base_hd->m_next_hid);
                    if (next_hd.get())
                    {
                        json_spirit::Object o;
                        o.push_back( Pair("hid", next_hd->m_hid));
                        o.push_back( Pair("name", next_hd->m_name));
                        o.push_back( Pair("spic", next_hd->m_spic));
                        o.push_back( Pair("star", next_hd->m_quality));
                        o.push_back( Pair("quality", next_hd->m_quality));
                        o.push_back( Pair("id", next_id));
                        epic_array.push_back(o);
                    }
                }
                else
                {
                    json_spirit::Object o;
                    o.push_back( Pair("hid", base_hd->m_hid));
                    o.push_back( Pair("name", base_hd->m_name));
                    o.push_back( Pair("spic", base_hd->m_spic));
                    o.push_back( Pair("star", base_hd->m_quality));
                    o.push_back( Pair("quality", base_hd->m_quality));
                    o.push_back( Pair("id", 0));
                    epic_array.push_back(o);
                }
            }
        }
    }
    robj.push_back( Pair("list", epic_array));
    return HC_SUCCESS;
}

//招募神将
int ProcessGetEpicHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return Singleton<HeroMgr>::Instance().GetEpicHero(robj,o,cdata);
}

//提升神将
int ProcessUpEpicHeroInfo(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.UpEpicHeroInfo(robj,o);
}

int ProcessUpEpicHero(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    return cdata->m_heros.UpEpicHero(robj,o);
}

void baseHeroData::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("hid", m_hid));
    obj.push_back( Pair("race", m_race));
    obj.push_back( Pair("name", m_name));
    obj.push_back( Pair("spic", m_spic));
    obj.push_back( Pair("memo", m_memo));
    obj.push_back( Pair("base_attack", m_base_attack));
    obj.push_back( Pair("base_defense", m_base_defense));
    obj.push_back( Pair("base_magic", m_base_magic));
    obj.push_back( Pair("base_hp", m_base_hp));
    obj.push_back( Pair("epic", m_epic));
    obj.push_back( Pair("next_hid", m_next_hid));
    if (m_epic)
    {
        obj.push_back( Pair("base_attack_add", m_epic_data.m_add[0]));
        obj.push_back( Pair("base_defense_add", m_epic_data.m_add[1]));
        obj.push_back( Pair("base_magic_add", m_epic_data.m_add[2]));
        obj.push_back( Pair("base_hp_add", m_epic_data.m_add[3]));
    }
    return;
}

void baseHeroData::toEpicObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_hid));
    obj.push_back( Pair("star", m_quality));
    obj.push_back( Pair("level", 1));
    obj.push_back( Pair("exp", 0));
    obj.push_back( Pair("quality", m_quality));
    if (m_next_hid > 0)
    {
        json_spirit::Object tmp;
        Item item(ITEM_TYPE_GEM, m_epic_data.m_fragment_id, m_epic_data.m_get_cost, 0);
        item.toObj(tmp);
        obj.push_back( Pair("fragment_info", tmp));
        obj.push_back( Pair("get_level", m_epic_data.m_get_level));
        obj.push_back( Pair("get_vip", m_epic_data.m_get_vip));
    }
    toObj(obj);
    return;
}

void baseHeroData::loadMeterial()
{
    //init
    for (int star = 1; star <= iMaxHeroStar; ++star)
    {
        m_material[star-1].m_special_material_id = 0;
        m_material[star-1].m_decompose_special_num = 0;
        m_material[star-1].m_smelt_special_num = 0;
        for (int i = 0; i < 3; ++i)
        {
            m_material[star-1].m_decompose_material[i] = 0;
        }
        for (int i = 0; i < 3; ++i)
        {
            m_material[star-1].m_smelt_material[i] = 0;
        }
    }
    Query q(GetDb());
    q.get_result("SELECT star,special_material,decompose_num,smelt_num,decompose1,decompose2,decompose3,smelt1,smelt2,smelt3 FROM base_hero_meterial WHERE hid=" + LEX_CAST_STR(m_hid) + " order by star");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int star = q.getval();
        if (star < 1 || star > iMaxHeroStar)
        {
            continue;
        }
        m_material[star-1].m_special_material_id = q.getval();
        m_material[star-1].m_decompose_special_num = q.getval();
        m_material[star-1].m_smelt_special_num = q.getval();
        for (int i = 0; i < 3; ++i)
        {
            m_material[star-1].m_decompose_material[i] = q.getval();
        }
        for (int i = 0; i < 3; ++i)
        {
            m_material[star-1].m_smelt_material[i] = q.getval();
        }
    }
    q.free_result();
    return;
}

void baseHeroData::loadEpicData()
{
    //init
    m_epic_data.m_fragment_id = 0;
    m_epic_data.m_scroll_id = 0;
    m_epic_data.m_get_cost = 0;
    m_epic_data.m_get_level = 0;
    m_epic_data.m_get_vip = 0;
    m_epic_data.m_up_cost = 0;
    m_epic_data.m_up_level = 0;
    m_epic_data.m_up_vip = 0;
    for (int i = 0; i < 4; ++i)
    {
        m_epic_data.m_add[i] = 0.0;
    }
    if (m_epic)
    {
        Query q(GetDb());
        if (m_next_hid)
        {
            q.get_result("SELECT fragment_id,scroll_id,get_cost,get_level,get_vip,up_cost,up_level,up_vip FROM base_hero_epic WHERE hid=" + LEX_CAST_STR(m_hid));
            CHECK_DB_ERR(q);
            if (q.fetch_row())
            {
                m_epic_data.m_fragment_id = q.getval();
                m_epic_data.m_scroll_id = q.getval();
                m_epic_data.m_get_cost = q.getval();
                m_epic_data.m_get_level = q.getval();
                m_epic_data.m_get_vip = q.getval();
                m_epic_data.m_up_cost = q.getval();
                m_epic_data.m_up_level = q.getval();
                m_epic_data.m_up_vip = q.getval();
            }
            q.free_result();
        }
        q.get_result("SELECT attack_add,defense_add,magic_add,hp_add FROM base_hero_epic_add WHERE hid=" + LEX_CAST_STR(m_hid));
        CHECK_DB_ERR(q);
        if (q.fetch_row())
        {
            for (int i = 0; i < 4; ++i)
            {
                m_epic_data.m_add[i] = q.getnum();
            }
        }
        q.free_result();
    }
    return;
}

void smeltHeroData::refresh()
{
    m_hid = Singleton<HeroMgr>::Instance().RandomHero();
    //m_star = my_random(1,5);
    int m_gailvs[5] = {32,30,30,7,1};
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(m_gailvs);
    m_star = dist(gen) + 1;
    m_baseHero = Singleton<HeroMgr>::Instance().GetBaseHero(m_hid);
    InsertSaveDb("replace into char_smelt_heros (cid,pos,hid,star) values ("
                    + LEX_CAST_STR(m_cid)
                    + "," + LEX_CAST_STR(m_pos)
                    + "," + LEX_CAST_STR(m_hid)
                    + "," + LEX_CAST_STR(m_star) + ")");
    return;
}

int CharHeroData::addExp(int exp, int statistics_type)
{
    bool level_up = false;
    if (exp < 0)
        return -1;
    m_exp += exp;
    if (m_level >= iMaxLevel)
        m_exp = 0;
    int need_exp = GeneralDataMgr::getInstance()->GetBaseExp(m_level + 1);
    while (need_exp > 0)
    {
        if (m_exp < need_exp)
            break;
        m_exp -= need_exp;
        ++m_level;
        need_exp = GeneralDataMgr::getInstance()->GetBaseExp(m_level + 1);
        level_up = true;
    }
    if (level_up)
    {
    }
    //更新属性
    m_changed = true;
    updateAttribute();
    statistics_of_hero_exp_get(m_belong_to.m_charData.m_id, m_belong_to.m_charData.m_ip_address, exp, statistics_type, m_belong_to.m_charData.m_union_id, m_belong_to.m_charData.m_server_id);
    return 0;
}

//武将升级
int CharHeroData::levelup(int level)
{
    if (level >= iMaxLevel)
        level = iMaxLevel;
    if (m_level < level)
    {
        m_level = level;
        //更新属性
        m_changed = true;
        updateAttribute();
        return 0;
    }
    return -1;
}

//穿上装备
int CharHeroData::equipt(int slot, int eid)
{
    //cout<<"CharHeroData::equip(),slot:"<<(int)slot<<",eid:"<<eid<<endl;
    boost::shared_ptr<item_base> eqq = m_belong_to.m_charData.m_bag.getEquipItemById(eid);
    //没这件装备
    if (!eqq.get())
    {
        return HC_ERROR;
    }
    Equipment* equipt = dynamic_cast<Equipment*>(eqq.get());
    //slot是否正确
    if (slot > EQUIP_SLOT_MAX || slot < EQUIP_SLOT_WEAPON)
    {
        slot = equipt->getEquipType();//现在 类型就是装备位置
    }

    //只能在背包中
    if (equipt->getContainer() != &m_belong_to.m_charData.m_bag)
    {
        return HC_ERROR;
    }
    //从背包中移除
    m_belong_to.m_charData.m_bag.removeItem(equipt->getSlot());

    //原先的位置上是否有装备
    int add_pre = 0, add_now = 0;
    boost::shared_ptr<item_base> itm = m_bag.getItem(slot);
    if (itm.get())
    {
        cout<<"CharHeroData::equip(), remove item slot:"<<(int)slot<<",eid:"<<eid<<endl;
        m_bag.removeItem(slot);
        m_belong_to.m_charData.m_bag.addItem(itm);
        Equipment* ed = dynamic_cast<Equipment*>(itm.get());
        ed->Save();
        add_pre = ed->getValue();
    }

    cout<<"m_bag.addItem(itm)"<<endl;
    //装备上
    m_bag.addItem(slot, eqq);
    equipt->Save();
    add_now = equipt->getValue();

    updateAttribute();

    //通知战斗属性变化
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "equip_change") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("type", equipt->getEquipType()) );
    obj.push_back( Pair("add_pre", add_pre) );
    obj.push_back( Pair("add_now", add_now) );
    m_belong_to.m_charData.sendObj(obj);

    //活动更新
    if (Singleton<actionMgr>::Instance().isEquiptLevelActionOpen(&(m_belong_to.m_charData)))
    {
        Singleton<actionMgr>::Instance().updateEquiptLevelAction(this);
    }

    //目标
    Singleton<goalMgr>::Instance().updateTask(m_belong_to.m_charData.m_id, GOAL_TYPE_EQUIPT1_QUALITY+slot-1, equipt->getQuality());
    Singleton<goalMgr>::Instance().updateTask(m_belong_to.m_charData.m_id, GOAL_TYPE_EQUIPT_QUALITY+slot-1, equipt->getQuality());

    return HC_SUCCESS;
}

//卸下装备
int CharHeroData::unequipt(int slot)
{
    //cout<<"CharHeroData::unequip(),slot:"<<(int)slot<<",gid:"<<m_id<<endl;
    //包裹满了
    if (m_belong_to.m_charData.m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    if (slot > EQUIP_SLOT_MAX || slot < EQUIP_SLOT_WEAPON)
    {
        return HC_ERROR;
    }
    boost::shared_ptr<item_base> itm = m_bag.getItem(slot);
    if (!itm.get() || itm->getType() != ITEM_TYPE_EQUIPMENT)
    {
        return HC_ERROR;
    }
    int add_pre = 0;
    Equipment* eq = dynamic_cast<Equipment*>(itm.get());
    m_bag.removeItem(slot);
    m_belong_to.m_charData.m_bag.addItem(itm);
    eq->Save();
    add_pre = eq->getValue();

    updateAttribute();

    //通知战斗属性变化
    json_spirit::Object obj;
    obj.push_back( Pair("cmd", "equip_change") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("type", eq->getEquipType()) );
    obj.push_back( Pair("add_pre", add_pre) );
    obj.push_back( Pair("add_now", 0) );
    m_belong_to.m_charData.sendObj(obj);
    return HC_SUCCESS;
}

int CharHeroData::useGem(int tid, int nums)
{
    boost::shared_ptr<baseGem> bt = GeneralDataMgr::getInstance()->GetBaseGem(tid);
    if (!bt.get())
    {
        return HC_ERROR;
    }
    int has_num = m_belong_to.m_charData.m_bag.getGemCount(tid);
    int use_num = has_num;
    if (nums > 0 && has_num > nums)
    {
        use_num = nums;
    }
    else if (nums <= 0 || nums > has_num)
    {
        return HC_ERROR;
    }
    switch (bt->usage)
    {
        case GEM_USAGE_HERO_EXP:
        {
            if (bt->extra[0] <= 0)
            {
                return HC_ERROR;
            }
            int add = 0;
            int count = use_num;
            if (count <= 0)
            {
                count = 1;
            }
            add = bt->extra[0] * count;
            m_belong_to.m_charData.subGem(tid,count,gem_cost_use);
            addExp(add, hero_exp_get_gem);
            return HC_SUCCESS;
        }
    }
    return HC_ERROR;
}

void CharHeroData::updateAttribute(bool real_update)
{
    if (m_baseHero.get())
    {
        m_init_attr = true;
        //各种百分比加成累加效果
        int attack_fac = 0, defense_fac = 0, magic_fac = 0, hp_fac = 0;
        //玩家加成
        if (m_belong_to.m_charData.m_race_data.get())
        {
            int char_attack_fac = 0, char_defense_fac = 0, char_magic_fac = 0;
            if (m_belong_to.m_charData.m_race_data->getLevelAdd(m_belong_to.m_charData.m_level, char_attack_fac, char_defense_fac, char_magic_fac))
            {
                attack_fac += char_attack_fac;
                defense_fac += char_defense_fac;
                magic_fac += char_magic_fac;
            }
        }
        //城堡加成
        char_castle* cc = Singleton<cityMgr>::Instance().getCharCastle(m_belong_to.m_charData.m_id);
        if (cc)
        {
            int castle_attack_fac = 0, castle_defense_fac = 0, castle_levy_fac = 0;
            cc->cal_add(castle_attack_fac,castle_defense_fac,castle_levy_fac);
            attack_fac += castle_attack_fac;
            defense_fac += castle_defense_fac;
            if (m_state == HERO_STATE_CITY)
            {
                int d_attack_fac = 0, d_defense_fac = 0, d_magic_fac = 0, d_hp_fac;
                cc->getDefenseAdd(d_attack_fac,d_defense_fac,d_magic_fac,d_hp_fac);
                attack_fac += d_attack_fac;
                defense_fac += d_defense_fac;
                magic_fac += d_magic_fac;
                hp_fac += d_hp_fac;
            }
        }
        //限时增益
        {
            attack_fac += m_belong_to.m_charData.m_Buffs.buffs[0].m_value;
            defense_fac += m_belong_to.m_charData.m_Buffs.buffs[1].m_value;
            magic_fac += m_belong_to.m_charData.m_Buffs.buffs[2].m_value;
            hp_fac += m_belong_to.m_charData.m_Buffs.buffs[3].m_value;
        }
        /********************英雄基础属性**********************/
        //每级加成
        double attack_add = 0.0, defense_add = 0.0, magic_add = 0.0, hp_add = 0.0;
        attack_add = (double)m_baseHero->m_base_attack * m_add[0];
        defense_add = (double)m_baseHero->m_base_defense * m_add[1];
        magic_add = (double)m_baseHero->m_base_magic * m_add[2];
        hp_add = (double)m_baseHero->m_base_hp * m_add[3];
        //英雄基础属性
        m_hero_attack = m_baseHero->m_base_attack + attack_add * (m_level - 1);
        m_hero_defense = m_baseHero->m_base_defense + defense_add * (m_level - 1);
        m_hero_magic = m_baseHero->m_base_magic + magic_add * (m_level - 1);
        m_hero_hp = m_baseHero->m_base_hp + hp_add * (m_level - 1);

        //各种百分比加成只加成英雄
        m_attack = m_hero_attack * (100 + attack_fac) / 100;
        m_defense = m_hero_defense * (100 + defense_fac) / 100;
        m_magic = m_hero_magic * (100 + magic_fac) / 100;
        m_hp = m_hero_hp * (100 + hp_fac) / 100;

        /********************数值累加**********************/
        //装备其他加成
        int equipt_attack = 0, equipt_defense = 0, equipt_magic = 0, equipt_hp = 0;
        for (int i = 1; i <= EQUIP_SLOT_MAX; ++i)
        {
            Equipment* eq = dynamic_cast<Equipment*>(m_bag.getItem(i).get());
            if (eq)
            {
                switch (eq->getEquipType())
                {
                    case EQUIP_SLOT_WEAPON:
                        equipt_attack += eq->getValue();
                        break;
                    case EQUIP_SLOT_SHIELD:
                        equipt_defense += eq->getValue();
                        break;
                    case EQUIP_SLOT_CLOTH:
                        equipt_hp += eq->getValue();
                        break;
                    case EQUIP_SLOT_BOOK:
                        equipt_magic += eq->getValue();
                        break;
                }
            }
        }
        m_attack += equipt_attack;
        m_defense += equipt_defense;
        m_magic += equipt_magic;
        m_hp += equipt_hp;
        //公会技能加成
        if (m_belong_to.m_charData.m_guild_data.get() && m_belong_to.m_charData.m_guild_forever_data.get())
        {
            int guild_attack = 0, guild_defense = 0, guild_magic = 0, guild_hp = 0;
            m_belong_to.m_charData.m_guild_forever_data->getSkillAdd(guild_attack, guild_defense, guild_magic, guild_hp);
            m_attack += guild_attack;
            m_defense += guild_defense;
            m_magic += guild_magic;
            m_hp += guild_hp;
        }
        //技能加成
        m_attack += m_belong_to.m_charData.m_combat_attribute.skill_attack_add();
        m_defense+= m_belong_to.m_charData.m_combat_attribute.skill_defense_add();
        m_magic += m_belong_to.m_charData.m_combat_attribute.skill_magic_add();
        if (real_update)
        {
            Save();
            int tmp = m_attack+m_defense+m_magic;
            if (m_attribute != tmp)
            {
                m_attribute = tmp;
                InsertSaveDb("update char_heros set attribute = "+ LEX_CAST_STR(m_attribute) + " where id=" + LEX_CAST_STR(m_id));
            }
            if (isDefault())
            {
                //通知玩家出战英雄属性变动
                boost::shared_ptr<OnlineCharactor> account = GeneralDataMgr::getInstance()->GetOnlineCharactor(m_belong_to.m_charData.m_name);
                if (account.get())
                {
                    json_spirit::Object robj;
                    robj.push_back( Pair("cmd", "getHeroInfo") );
                    robj.push_back( Pair("s", 200) );
                    json_spirit::Object hero_obj;
                    toObj(hero_obj);
                    robj.push_back( Pair("info", hero_obj));
                    robj.push_back( Pair("purpose", 3));
                    account->Send(json_spirit::write(robj, json_spirit::raw_utf8));
                }
                m_belong_to.m_charData.NotifyCharData();
            }
        }
    }
}

int CharHeroData::Save()
{
    if (m_changed)
    {
        //保存到数据库
        InsertSaveDb("update char_heros set level=" + LEX_CAST_STR(m_level)
                    + ",star=" + LEX_CAST_STR(m_star)
                    + ",add_fac1=" + LEX_CAST_STR(m_add[0])
                    + ",add_fac2=" + LEX_CAST_STR(m_add[1])
                    + ",add_fac3=" + LEX_CAST_STR(m_add[2])
                    + ",add_fac4=" + LEX_CAST_STR(m_add[3])
                    + ",hero_attack=" + LEX_CAST_STR(m_hero_attack)
                    + ",hero_defense=" + LEX_CAST_STR(m_hero_defense)
                    + ",hero_magic=" + LEX_CAST_STR(m_hero_magic)
                    + ",hero_hp=" + LEX_CAST_STR(m_hero_hp)
                    + ",exp=" + LEX_CAST_STR(m_exp)
                    + ",quality=" + LEX_CAST_STR(m_quality)
                    + ",state=" + LEX_CAST_STR(m_state)
                    + " where id=" + LEX_CAST_STR(m_id)
        );
        m_changed = false;
    }
    return 0;
}

void CharHeroData::toObj(json_spirit::Object& obj, int star)
{
    if (!m_baseHero.get())
    {
        return;
    }
    if (!m_init_attr)
    {
        updateAttribute();
    }
    //虚拟特定星级对象
    int org_star = 0;
    if (star != 0)
    {
        org_star = m_star;
        m_star = star;
        m_quality = m_star;
        updateAttribute(false);
    }
    obj.push_back( Pair("id", m_id));
    obj.push_back( Pair("cid", m_belong_to.m_charData.m_id));
    obj.push_back( Pair("star", m_star));
    obj.push_back( Pair("attack_add", m_add[0]));
    obj.push_back( Pair("defense_add", m_add[1]));
    obj.push_back( Pair("magic_add", m_add[2]));
    obj.push_back( Pair("hp_add", m_add[3]));
    obj.push_back( Pair("quality", m_quality));
    obj.push_back( Pair("level", m_level));
    obj.push_back( Pair("state", m_state));
    obj.push_back( Pair("exp", m_exp));
    if (m_level < iMaxLevel)
        obj.push_back( Pair("need_exp", GeneralDataMgr::getInstance()->GetBaseExp(m_level + 1)));
    m_baseHero->toObj(obj);
    //总战斗属性
    obj.push_back( Pair("attack", m_attack));
    obj.push_back( Pair("defense", m_defense));
    obj.push_back( Pair("magic", m_magic));
    obj.push_back( Pair("hp", m_hp));
    //英雄自身战斗属性
    obj.push_back( Pair("hero_attack", m_hero_attack));
    obj.push_back( Pair("hero_defense", m_hero_defense));
    obj.push_back( Pair("hero_magic", m_hero_magic));
    obj.push_back( Pair("hero_hp", m_hero_hp));
    //虚拟星级后换回正常星级
    if (org_star != 0)
    {
        m_star = org_star;
        m_quality = m_star;
        updateAttribute(false);
    }
    return;
}

void CharHeroData::updateStar(int up_star)
{
    if (up_star < 1)
        up_star = 1;
    if (up_star > iMaxHeroStar)
        up_star = iMaxHeroStar;
    m_star = up_star;
    if (m_baseHero.get() && m_baseHero->m_epic == 0)
    {
        m_add[0] = my_random(iHeroStarAdd[m_star-1] - 0.2,iHeroStarAdd[m_star-1] + 0.2);
        m_add[1] = my_random(iHeroStarAdd[m_star-1] - 0.2,iHeroStarAdd[m_star-1] + 0.2);
        m_add[2] = my_random(iHeroStarAdd[m_star-1] - 0.2,iHeroStarAdd[m_star-1] + 0.2);
        m_add[3] = my_random(iHeroStarAdd[m_star-1] - 0.2,iHeroStarAdd[m_star-1] + 0.2);
    }
    m_quality = m_star;
    m_changed = true;
    updateAttribute();
}

bool CharHeroData::checkEquiptLevel(int level)
{
    for (size_t i = 0; i < m_bag.m_size; ++i)
    {
        if (m_bag.m_bagslot[i].get() && m_bag.m_bagslot[i]->getType() == ITEM_TYPE_EQUIPMENT)
        {
            Equipment* p = dynamic_cast<Equipment*>(m_bag.m_bagslot[i].get());
            if (p->getLevel() < level)
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    return true;
}

//从数据库中加载英雄
int CharTotalHeros::Load()
{
    Query q(GetDb());
    q.get_result("SELECT id,hid,level,star,add_fac1,add_fac2,add_fac3,add_fac4,exp,quality,state,attribute FROM char_heros WHERE cid=" + LEX_CAST_STR(m_cid) + " order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        boost::shared_ptr<CharHeroData> hd;
        hd.reset(new CharHeroData(m_charData, *this));
        hd->m_id = q.getval();
        hd->m_cid = m_cid;
        hd->m_hid = q.getval();
        hd->m_level = q.getval();
        hd->m_star = q.getval();
        hd->m_add[0] = q.getnum();
        hd->m_add[1] = q.getnum();
        hd->m_add[2] = q.getnum();
        hd->m_add[3] = q.getnum();
        hd->m_exp = q.getval();
        hd->m_quality = q.getval();
        hd->m_state = q.getval();
        hd->m_attribute = q.getval();
        hd->m_baseHero = Singleton<HeroMgr>::Instance().GetBaseHero(hd->m_hid);
        if (!hd->m_baseHero.get())
        {
            continue;
        }
        hd->m_spic = hd->m_baseHero->m_spic;
        hd->m_race = hd->m_baseHero->m_race;
        m_heros[hd->m_id] = hd;
        if (hd->m_state == HERO_STATE_DEFAULT)
        {
            m_default_hero = hd->m_id;
        }
        hd->m_bag.loadBag();
    }
    q.free_result();
    //英雄属性更新不能在加载的时候调用
    //updateAttribute();

    q.get_result("SELECT pos,hid,star FROM char_smelt_heros WHERE cid=" + LEX_CAST_STR(m_cid) + " order by pos");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int pos = q.getval();
        if (pos < 1 || pos > iSmeltMaxCnt)
            continue;
        boost::shared_ptr<smeltHeroData> s_hero;
        s_hero.reset(new smeltHeroData());
        s_hero->m_cid = m_cid;
        s_hero->m_pos = pos;
        s_hero->m_hid = q.getval();
        s_hero->m_star = q.getval();
        s_hero->m_baseHero = Singleton<HeroMgr>::Instance().GetBaseHero(s_hero->m_hid);
        if (!s_hero->m_baseHero.get())
        {
            ERR();
        }
        else
        {
            m_smeltHeros[pos-1] = s_hero;
        }
    }
    q.free_result();
    return 0;
}

size_t CharTotalHeros::addSize(size_t a)
{
    if (a + m_hero_max <= MAX_HERO_SIZE)
    {
        m_hero_max += a;
    }
    else
    {
        m_hero_max = MAX_HERO_SIZE;
    }
    return m_hero_max;
}

//购买英雄位置
int CharTotalHeros::buyHeroSize(int num, json_spirit::Object& robj)
{
    if (num < 0)
    {
        num = 1;
    }
    if (num + m_hero_max > MAX_HERO_SIZE)
    {
        num = MAX_HERO_SIZE - m_hero_max;
        if (num < 1)
        {
            return HC_ERROR;
        }
    }

    int buyed = m_hero_max - HERO_DEFAULT_SIZE;
    int gold_need = 0;
    for (int i = buyed + 1; i <= (buyed+num); ++i)
    {
        int tmp = ((i-1) / 3 + 1) * 10;
        if (tmp >= 50)
            tmp = 50;
        gold_need += tmp;
    }
    if (m_charData.subGold(gold_need, gold_cost_buy_hero_size) >= 0)
    {
        addSize(num);
        //保存背包购买
        InsertSaveDb("update char_data set heroSize=" + LEX_CAST_STR(buyed+num) + " where cid=" + LEX_CAST_STR(m_charData.m_id));
        return HC_SUCCESS;
    }
    else
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
}

//获得英雄
int CharTotalHeros::Add(int id, int level, int star, bool set_add, double add1, double add2, double add3, double add4)
{
    boost::shared_ptr<baseHeroData> bhd = Singleton<HeroMgr>::Instance().GetBaseHero(id);
    if (!bhd.get())
    {
        return 0;
    }
    int quality = star;
    int hid = GeneralDataMgr::getInstance()->newHeroId();

    boost::shared_ptr<CharHeroData> hd;
    hd.reset(new CharHeroData(m_charData, *this));
    hd->m_id = hid;
    hd->m_cid = m_cid;
    hd->m_hid = id;
    hd->m_spic = bhd->m_spic;
    hd->m_race = bhd->m_race;
    hd->m_star = star;
    if (bhd->m_epic)
    {
        hd->m_add[0] = bhd->m_epic_data.m_add[0];
        hd->m_add[1] = bhd->m_epic_data.m_add[1];
        hd->m_add[2] = bhd->m_epic_data.m_add[2];
        hd->m_add[3] = bhd->m_epic_data.m_add[3];
    }
    else
    {
        hd->m_add[0] = set_add ? add1 : my_random(iHeroStarAdd[star-1] - 0.2,iHeroStarAdd[star-1] + 0.2);
        hd->m_add[1] = set_add ? add2 : my_random(iHeroStarAdd[star-1] - 0.2,iHeroStarAdd[star-1] + 0.2);
        hd->m_add[2] = set_add ? add3 : my_random(iHeroStarAdd[star-1] - 0.2,iHeroStarAdd[star-1] + 0.2);
        hd->m_add[3] = set_add ? add4 : my_random(iHeroStarAdd[star-1] - 0.2,iHeroStarAdd[star-1] + 0.2);
    }
    hd->m_level = level;
    hd->m_quality = quality;
    hd->m_attack = bhd->m_base_attack;
    hd->m_defense = bhd->m_base_defense;
    hd->m_magic = bhd->m_base_magic;
    hd->m_hp = bhd->m_base_hp;
    hd->m_baseHero = bhd;
    hd->m_state = 0;
    if (m_default_hero == 0)
    {
        m_default_hero = hd->m_id;
        hd->m_state = HERO_STATE_DEFAULT;
    }
    hd->updateAttribute();
    //加入玩家英雄队列
    m_heros[hd->m_id] = hd;
    //加入英雄管理
    GeneralDataMgr::getInstance()->setHeroOwner(hid, m_cid);
    //更新任务
    m_charData.m_tasks.updateTask(GOAL_HERO_STAR, star, 1);
    m_charData.m_tasks.updateTask(GOAL_HERO, id, 1);
    Singleton<goalMgr>::Instance().updateTask(m_charData.m_id, GOAL_TYPE_HERO_CNT, m_heros.size());
    Singleton<goalMgr>::Instance().updateTask(m_charData.m_id, GOAL_TYPE_HERO_STAR, star);
    if (bhd->m_epic)
    {
        Singleton<goalMgr>::Instance().updateTask(m_charData.m_id, GOAL_TYPE_EPIC_HERO, 1);
    }
    //活动更新
    if (Singleton<actionMgr>::Instance().isHeroActionOpen(&m_charData))
    {
        Singleton<actionMgr>::Instance().updateHeroAction(m_cid, id);
    }
    InsertSaveDb("insert into char_heros (id,cid,hid,level,star,add_fac1,add_fac2,add_fac3,add_fac4,quality,state) values ("
                    + LEX_CAST_STR(hid)
                    + "," + LEX_CAST_STR(m_cid)
                    + "," + LEX_CAST_STR(id)
                    + "," + LEX_CAST_STR(level)
                    + "," + LEX_CAST_STR(star)
                    + "," + LEX_CAST_STR(hd->m_add[0])
                    + "," + LEX_CAST_STR(hd->m_add[1])
                    + "," + LEX_CAST_STR(hd->m_add[2])
                    + "," + LEX_CAST_STR(hd->m_add[3])
                    + "," + LEX_CAST_STR(quality)
                    + "," + LEX_CAST_STR(hd->m_state) + ")");
    return hid;
}

//消耗英雄
int CharTotalHeros::Sub(int id)
{
    if (m_default_hero == id)
    {
        return -1;
    }
    boost::shared_ptr<CharHeroData> phd = GetHero(id);
    if (!phd.get())
    {
        return -1;
    }
    //英雄身上装备
    for (size_t i = 1; i <= EQUIP_SLOT_MAX; ++i)
    {
        phd->unequipt(i);
    }
    //移出玩家英雄队列
    m_heros.erase(id);
    //删除英雄管理
    GeneralDataMgr::getInstance()->removeHeroOwner(id);
    InsertSaveDb("delete from char_heros where id=" + LEX_CAST_STR(id));
    return 0;
}

//根据id取英雄
boost::shared_ptr<CharHeroData> CharTotalHeros::GetHero(int id)
{
    if (id > 0)
    {
        std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.find(id);
        if (it != m_heros.end())
        {
            return it->second;
        }
    }
    else if (id == 0)
    {
        std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.begin();
        if (it != m_heros.end())
        {
            return it->second;
        }
    }
    boost::shared_ptr<CharHeroData> hd;
    hd.reset();
    return hd;
}

//根据基础类型取英雄
int CharTotalHeros::GetHeroByType(int htype)
{
    std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.begin();
    while (it != m_heros.end())
    {
        if (it->second.get() && it->second->m_hid == htype)
        {
            return it->second->m_id;
        }
        ++it;
    }
    return 0;
}

Equipment* CharTotalHeros::getEquipById(int id)
{
    Equipment* eq = NULL;
    for (std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.begin(); it != m_heros.end(); ++it)
    {
        if (it->second.get())
        {
            eq = it->second->m_bag.getEquipById(id);
            if (eq)
            {
                return eq;
            }
        }
    }
    return NULL;
}

//设置出战英雄
int CharTotalHeros::SetDefault(int hid, json_spirit::Object& obj)
{
    if (hid <= 0)
        return HC_ERROR;
    if (m_default_hero != hid)
    {
        //设置新英雄状态
        boost::shared_ptr<CharHeroData> new_hero = GetHero(hid);
        if (!new_hero.get())
        {
            return HC_ERROR;
        }
        if ((new_hero->m_star > 3 && m_charData.m_level <= 30)
            || (new_hero->m_star > 4 && m_charData.m_level <= 40))
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
        //原先出战英雄状态变更
        if (m_default_hero > 0)
        {
            boost::shared_ptr<CharHeroData> old_hero = GetHero(m_default_hero);
            if (old_hero.get())
            {
                old_hero->m_state = HERO_STATE_INIT;
                old_hero->m_changed = true;
                old_hero->Save();
            }
        }
        //设置新英雄状态
        new_hero->m_state = HERO_STATE_DEFAULT;
        new_hero->m_changed = true;
        new_hero->Save();
        m_default_hero = hid;
        obj.push_back( Pair("id", hid));
        //活动更新
        if (Singleton<actionMgr>::Instance().isEquiptLevelActionOpen(&m_charData))
        {
            Singleton<actionMgr>::Instance().updateEquiptLevelAction(new_hero.get());
        }
        m_charData.NotifyCharData();
        boost::shared_ptr<CharPkData> charPk = Singleton<PkMgr>::Instance().getPkData(m_charData.m_id);
        if (charPk.get() && charPk->getCharData().get() && charPk->m_roomid > 0)
        {
            Singleton<PkMgr>::Instance().broadInfo(charPk->m_roomid);
        }
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

//获取简单的英雄出战列表
void CharTotalHeros::getList(json_spirit::Array& hlist)
{
    //默认出战英雄放第一
    json_spirit::Object obj;
    boost::shared_ptr<CharHeroData> p_hero = GetHero(m_default_hero);
    if (p_hero.get())
    {
        obj.push_back( Pair("id", m_default_hero) );
        obj.push_back( Pair("spic", p_hero->m_spic) );
        obj.push_back( Pair("level", p_hero->m_level) );
        obj.push_back( Pair("race", p_hero->m_race) );
        obj.push_back( Pair("star", p_hero->m_star) );
        obj.push_back( Pair("quality", p_hero->m_quality) );
        obj.push_back( Pair("name", p_hero->m_baseHero->m_name) );
    }
    hlist.push_back(obj);
    //其他守城英雄
}

int CharTotalHeros::CompoundHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int star = 0, cnt = 0, update_hero = 0;
    READ_INT_FROM_MOBJ(star,o,"star");
    READ_INT_FROM_MOBJ(cnt,o,"count");
    READ_INT_FROM_MOBJ(update_hero,o,"update_hero");
    boost::shared_ptr<CharHeroData> up_hero = GetHero(update_hero);
    if (!up_hero.get() || !up_hero->m_baseHero.get() || up_hero->m_star >= iMaxHeroStar || up_hero->m_baseHero->m_epic > 0)
    {
        ERR();
        return HC_ERROR;
    }
    //炼金房等级决定可合成星级上限
    int c_star = cm->getCompoundMaxStar();
    if (c_star < star + 1)
    {
        return HC_ERROR_NEED_MORE_METALLURGY_LEVEL;
    }
    json_spirit::Object hero_obj;
    up_hero->toObj(hero_obj,up_hero->m_star+1);
    obj.push_back( Pair("hero_obj", hero_obj));
    //基础加成和金币加成
    if (cnt >= 1 && cnt <= 8 && star >= 1 && star < iMaxHeroStar)
    {
        obj.push_back( Pair("base_per", iCompoundHeroPer[star-1][cnt-1]));
        obj.push_back( Pair("gold_cost", iCompoundHeroGoldPer[star-1][cnt-1][0]));
        obj.push_back( Pair("gold_per", iCompoundHeroGoldPer[star-1][cnt-1][1]));
        obj.push_back( Pair("silver_cost", iCompoundHeroSilver[star-1]));
    }
    else
    {
        obj.push_back( Pair("base_per", 0.0));
        obj.push_back( Pair("gold_cost", 0));
        obj.push_back( Pair("gold_per", 0.0));
        obj.push_back( Pair("silver_cost", 0));
    }
    //种族加成
    if (m_charData.m_race_data.get())
    {
        obj.push_back( Pair("race_per", m_charData.m_race_data->m_compound_add));
    }
    //炼金房加成
    obj.push_back( Pair("city_per", cm->getCompoundAdd()) );
    return HC_SUCCESS;
}

int CharTotalHeros::CompoundHero(json_spirit::Object& obj, json_spirit::mObject& o)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int hid = 0, race = 0, star = 0, rank = 0, cnt = 0, cost = 0, update_hero = 0;
    READ_INT_FROM_MOBJ(cost,o,"cost");
    READ_INT_FROM_MOBJ(update_hero,o,"update_hero");
    boost::shared_ptr<CharHeroData> up_hero = GetHero(update_hero);
    if (!up_hero.get() || !up_hero->m_baseHero.get() || up_hero->m_baseHero->m_epic > 0)
    {
        ERR();
        return HC_ERROR;
    }
    hid = up_hero->m_hid;
    race = up_hero->m_race;
    star = up_hero->m_star;
    rank = getLevelRank(up_hero->m_level);
    std::list<int> hero_list;
    json_spirit::mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"hero_list");
    json_spirit::mArray::iterator it = list.begin();
    while (it != list.end())
    {
        if ((*it).type() != json_spirit::obj_type)
        {
            ++it;
            continue;
        }
        json_spirit::mObject& tmp_obj = (*it).get_obj();
        int tmp_id = 0;
        READ_INT_FROM_MOBJ(tmp_id,tmp_obj,"id");
        boost::shared_ptr<CharHeroData> p_hero = GetHero(tmp_id);
        if (!p_hero.get() || !p_hero->m_baseHero.get() || p_hero->m_baseHero->m_epic > 0)
        {
            ERR();
            return HC_ERROR;
        }
        if (p_hero->isWork())
        {
            return HC_ERROR_DEFAULT_HERO;
        }
        if (race != p_hero->m_race || star != p_hero->m_star || rank != getLevelRank(p_hero->m_level))
        {
            ERR();
            return HC_ERROR;
        }
        hero_list.push_back(tmp_id);
        ++cnt;
        ++it;
    }
    //合成需要最少两张卡且不能是满星
    if (cnt < 1 || cnt > 8 || star < 1 || star >= iMaxHeroStar)
    {
        return HC_ERROR;
    }
    //炼金房等级决定可合成星级上限
    int c_star = cm->getCompoundMaxStar();
    if (c_star < star + 1)
    {
        return HC_ERROR_NEED_MORE_METALLURGY_LEVEL;
    }
    double per = iCompoundHeroPer[star-1][cnt-1];
    if (cost && iCompoundHeroGoldPer[star-1][cnt-1][0])
    {
        per += iCompoundHeroGoldPer[star-1][cnt-1][1];
    }
    //种族加成
    if (m_charData.m_race_data.get())
    {
        per += m_charData.m_race_data->m_compound_add;
    }
    //炼金房加成
    per += cm->getCompoundAdd();
    INFO("compoundPer=" << per <<endl);
    if (m_charData.silver() < iCompoundHeroSilver[star-1])
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    if (cost && m_charData.subGold(iCompoundHeroGoldPer[star-1][cnt-1][0], gold_cost_hero_compound) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    if (m_charData.subSilver(iCompoundHeroSilver[star-1], silver_cost_hero_compound) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    //合成
    for(std::list<int>::iterator it_h = hero_list.begin(); it_h != hero_list.end(); ++it_h)
    {
        if (Sub(*it_h) != 0)
        {
            ERR();
            INFO("Sub hid=" << *it_h <<endl);
        }
    }
    if (my_random(0.0,100.0) < per)
    {
        //升品质
        up_hero->updateStar(up_hero->m_star + 1);
        obj.push_back( Pair("result", 1));
        m_charData.m_tasks.updateTask(GOAL_COMPOUND_HERO, up_hero->m_star, 1);
        m_charData.m_tasks.updateTask(GOAL_DAILY_COMPOUND_HERO, up_hero->m_star, 1);
        //活动更新
        if (Singleton<actionMgr>::Instance().isHeroStarActionOpen(&m_charData))
        {
            Singleton<actionMgr>::Instance().updateHeroStarAction(m_charData.m_id,up_hero->m_star);
        }
        //广播好友祝贺
        if (up_hero->m_star == 4)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_COMPOUND_QUALITY4_HERO, 0, 0);
        }
        else if (up_hero->m_star == 5)
        {
            Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_COMPOUND_QUALITY5_HERO, 0, 0);
        }
    }
    else
    {
        obj.push_back( Pair("result", 0));
    }
    Singleton<goalMgr>::Instance().updateTask(m_charData.m_id, GOAL_TYPE_COMPOUND_HERO, 1);
    return HC_SUCCESS;
}

int CharTotalHeros::DecomposeHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int decompose_hero = 0, star = 0;
    READ_INT_FROM_MOBJ(decompose_hero,o,"decompose_hero");
    boost::shared_ptr<CharHeroData> de_hero = GetHero(decompose_hero);
    if (!de_hero.get() || !de_hero->m_baseHero.get() || de_hero->m_baseHero->m_epic > 0)
    {
        ERR();
        return HC_ERROR;
    }
    star = de_hero->m_star;
    if (star < 1 || star > iMaxHeroStar)
    {
        return HC_ERROR;
    }
    //炼金房等级决定可分解星级上限
    int c_star = cm->getDecomposeMaxStar();
    if (c_star < star)
    {
        return HC_ERROR_NEED_MORE_METALLURGY_LEVEL;
    }
    obj.push_back( Pair("silver_cost", iDecomposeHeroSilver[star-1]));
    material_data& data = de_hero->m_baseHero->m_material[star-1];
    json_spirit::Array list;
    for (int i = 0; i < 3; ++i)
    {
        if (data.m_smelt_material[i] > 0)
        {
            json_spirit::Object o;
            Item item(ITEM_TYPE_GEM, GEM_ID_HERO_MATERIAL_BEGIN + i + 1, data.m_decompose_material[i], 0);
            item.toObj(o);
            list.push_back(o);
        }
    }
    if (data.m_special_material_id > 0 && data.m_decompose_special_num > 0)
    {
        json_spirit::Object o;
        Item item(ITEM_TYPE_GEM, data.m_special_material_id, data.m_decompose_special_num, 0);
        item.toObj(o);
        list.push_back(o);
    }
    obj.push_back( Pair("list", list));
    return HC_SUCCESS;
}

int CharTotalHeros::DecomposeHero(json_spirit::mObject& o)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int decompose_hero = 0, star = 0;
    READ_INT_FROM_MOBJ(decompose_hero,o,"decompose_hero");
    boost::shared_ptr<CharHeroData> de_hero = GetHero(decompose_hero);
    if (!de_hero.get() || !de_hero->m_baseHero.get() || de_hero->isWork() || de_hero->m_baseHero->m_epic > 0)
    {
        ERR();
        return HC_ERROR;
    }
    star = de_hero->m_star;
    if (star < 1 || star > iMaxHeroStar)
    {
        return HC_ERROR;
    }
    //炼金房等级决定可分解星级上限
    int c_star = cm->getDecomposeMaxStar();
    if (c_star < star)
    {
        return HC_ERROR_NEED_MORE_METALLURGY_LEVEL;
    }
    if (m_charData.subSilver(iDecomposeHeroSilver[star-1], silver_cost_hero_decompose) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    m_charData.m_tasks.updateTask(GOAL_DECOMPOSE_HERO, de_hero->m_star, 1);
    m_charData.m_tasks.updateTask(GOAL_DAILY_DECOMPOSE_HERO, de_hero->m_star, 1);
    m_charData.m_score_tasks.updateTask(DAILY_SCORE_DECOMPOSE_HERO);
    Singleton<goalMgr>::Instance().updateTask(m_charData.m_id, GOAL_TYPE_DECOMPOSE_HERO, 1);
    //消耗英雄
    if (Sub(decompose_hero) != 0)
    {
        ERR();
        INFO("Sub hid=" << decompose_hero <<endl);
        return HC_ERROR;
    }
    material_data& data = de_hero->m_baseHero->m_material[star-1];
    //获得材料
    for (int i = 0; i < 3; ++i)
    {
        if (data.m_decompose_material[i] > 0)
        {
            m_charData.addGem(GEM_ID_HERO_MATERIAL_BEGIN + i + 1,data.m_decompose_material[i], gem_get_hero_decompose);
        }
    }
    if (data.m_special_material_id > 0 && data.m_decompose_special_num > 0)
    {
        m_charData.addGem(data.m_special_material_id,data.m_decompose_special_num, gem_get_hero_decompose);
    }
    return HC_SUCCESS;
}

int CharTotalHeros::SmeltHeroInfo(json_spirit::Object& obj)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int smelt_cnt = cm->getSmeltCnt();
    json_spirit::Array smelt_herolist;
    for (int i = 0; i < smelt_cnt; ++i)
    {
        if (!m_smeltHeros[i].get())
        {
            boost::shared_ptr<smeltHeroData> s_hero;
            s_hero.reset(new smeltHeroData());
            s_hero->m_cid = m_cid;
            s_hero->m_pos = i+1;
            s_hero->refresh();
            if (!s_hero->m_baseHero.get())
            {
                ERR();
            }
            else
            {
                m_smeltHeros[i] = s_hero;
            }
        }
        if (m_smeltHeros[i]->m_hid > 0 && m_smeltHeros[i]->m_baseHero.get())
        {
            json_spirit::Object o;
            json_spirit::Object hero_obj;
            m_smeltHeros[i]->m_baseHero->toObj(hero_obj);
            hero_obj.push_back( Pair("star", m_smeltHeros[i]->m_star));
            hero_obj.push_back( Pair("level", 1));
            o.push_back( Pair("pos", i+1));
            o.push_back( Pair("hero_obj", hero_obj));
            int star = m_smeltHeros[i]->m_star;
            material_data& data = m_smeltHeros[i]->m_baseHero->m_material[star-1];
            json_spirit::Array list;
            for (int j = 0; j < 3; ++j)
            {
                if (data.m_smelt_material[j] > 0)
                {
                    json_spirit::Object gem_obj;
                    Item item(ITEM_TYPE_GEM, GEM_ID_HERO_MATERIAL_BEGIN + j + 1, data.m_smelt_material[j], 0);
                    item.toObj(gem_obj);
                    int cur_num = m_charData.m_bag.getGemCount(GEM_ID_HERO_MATERIAL_BEGIN + j + 1);
                    gem_obj.push_back( Pair("cur_num", cur_num));
                    list.push_back(gem_obj);
                }
            }
            if (data.m_special_material_id > 0 && data.m_smelt_special_num > 0)
            {
                json_spirit::Object gem_obj;
                Item item(ITEM_TYPE_GEM, data.m_special_material_id, data.m_smelt_special_num, 0);
                item.toObj(gem_obj);
                int cur_num = m_charData.m_bag.getGemCount(data.m_special_material_id);
                gem_obj.push_back( Pair("cur_num", cur_num));
                list.push_back(gem_obj);
            }
            o.push_back( Pair("list", list));
            smelt_herolist.push_back(o);
        }
        else
        {
            m_smeltHeros[i]->refresh();
        }
    }
    for (int i = smelt_cnt+1; i <= iSmeltMaxCnt; ++i)
    {
        int need_level = Singleton<cityMgr>::Instance().getSmeltOpenLevel(i);
        if (need_level > 0)
        {
            json_spirit::Object o;
            o.push_back( Pair("pos", i));
            o.push_back( Pair("need_level", need_level));
            smelt_herolist.push_back(o);
        }
    }
    obj.push_back( Pair("smelt_herolist", smelt_herolist));
    time_t refresh_time = m_charData.queryExtraData(char_data_type_normal,char_data_normal_smelt_hero_refresh);
    obj.push_back( Pair("smelt_refresh_cd", refresh_time-time(NULL)));
    obj.push_back( Pair("smelt_refresh_gold", iSmeltRefreshGold));
    return HC_SUCCESS;
}

int CharTotalHeros::SmeltHeroRefresh()
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    time_t t_now = time(NULL);
    time_t refresh = m_charData.queryExtraData(char_data_type_normal,char_data_normal_smelt_hero_refresh);
    if (t_now < refresh)
    {
        if (m_charData.subGold(iSmeltRefreshGold, gold_cost_hero_smelt_refresh) < 0)
            return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    int smelt_cnt = cm->getSmeltCnt();
    for (int i = 0; i < smelt_cnt; ++i)
    {
        if (!m_smeltHeros[i].get())
            continue;
        m_smeltHeros[i]->refresh();
    }
    time_t next_refresh = time(NULL) + iSmeltRefreshCD;
    m_charData.setExtraData(char_data_type_normal,char_data_normal_smelt_hero_refresh,next_refresh);
    m_charData.updateTopButton(top_button_smelt_refresh, 0);
    if (!cm->_uuid.is_nil())
    {
        splsTimerMgr::getInstance()->delTimer(cm->_uuid);
        cm->_uuid = boost::uuids::nil_uuid();
    }
    json_spirit::mObject mobj;
    mobj["cmd"] = "smeltRefreshUpdate";
    mobj["cid"] = m_charData.m_id;
    boost::shared_ptr<splsTimer> tmsg;
    tmsg.reset(new splsTimer(iSmeltRefreshCD, 1,mobj,1));
    cm->_uuid = splsTimerMgr::getInstance()->addTimer(tmsg);
    return HC_SUCCESS;
}

int CharTotalHeros::SmeltHero(json_spirit::mObject& o, json_spirit::Object& robj)
{
    int hid = 0, pos = 0;
    READ_INT_FROM_MOBJ(hid,o,"hid");
    READ_INT_FROM_MOBJ(pos,o,"pos");
    if (pos < 1 || pos > iSmeltMaxCnt)
        pos = 1;
    if (isFull())
        return HC_ERROR_HERO_FULL;
    if (!m_smeltHeros[pos-1].get())
        return HC_ERROR;
    if (m_smeltHeros[pos-1]->m_hid == hid && m_smeltHeros[pos-1]->m_baseHero.get())
    {
        material_data& data = m_smeltHeros[pos-1]->m_baseHero->m_material[m_smeltHeros[pos-1]->m_star-1];
        //判断材料
        for (int i = 0; i < 3; ++i)
        {
            if (data.m_smelt_material[i] > 0)
            {
                if (m_charData.m_bag.getGemCount(GEM_ID_HERO_MATERIAL_BEGIN + i + 1) < data.m_smelt_material[i])
                    return HC_ERROR_NOT_ENOUGH_GEM;
            }
        }
        if (data.m_special_material_id > 0 && data.m_smelt_special_num > 0)
        {
            if (m_charData.m_bag.getGemCount(data.m_special_material_id) < data.m_smelt_special_num)
                return HC_ERROR_NOT_ENOUGH_GEM;
        }
        //消耗材料
        for (int i = 0; i < 3; ++i)
        {
            if (data.m_smelt_material[i] > 0)
            {
                m_charData.subGem(GEM_ID_HERO_MATERIAL_BEGIN + i + 1,data.m_smelt_material[i],gem_cost_hero_smelt);
            }
        }
        if (data.m_special_material_id > 0 && data.m_smelt_special_num > 0)
        {
            m_charData.subGem(data.m_special_material_id,data.m_smelt_special_num,gem_cost_hero_smelt);
        }
        //增加英雄
        int new_id = Add(hid,1,m_smeltHeros[pos-1]->m_star);
        m_charData.m_tasks.updateTask(GOAL_SMELT_HERO, m_smeltHeros[pos-1]->m_star, 1);
        m_charData.m_score_tasks.updateTask(DAILY_SCORE_SMELT_HERO);
        robj.push_back( Pair("star", m_smeltHeros[pos-1]->m_star));
        if (new_id > 0)
        {
            std::string msg = "";
            std::string hero_link = "";
            switch (m_smeltHeros[pos-1]->m_star)
            {
                case 4:
                    msg = strSmeltHeroStar4Msg;
                    str_replace(msg, "$W", MakeCharNameLink(m_charData.m_name,m_charData.m_nick.get_string()));
                    hero_link = MakeHeroLink(m_smeltHeros[pos-1]->m_baseHero->m_name, m_charData.m_id, new_id, m_smeltHeros[pos-1]->m_star);
                    addColor(hero_link,m_smeltHeros[pos-1]->m_star);
                    str_replace(msg, "$R", hero_link);
                    break;
                case 5:
                    msg = strSmeltHeroStar5Msg;
                    str_replace(msg, "$W", MakeCharNameLink(m_charData.m_name,m_charData.m_nick.get_string()));
                    hero_link = MakeHeroLink(m_smeltHeros[pos-1]->m_baseHero->m_name, m_charData.m_id, new_id, m_smeltHeros[pos-1]->m_star);
                    addColor(hero_link,m_smeltHeros[pos-1]->m_star);
                    str_replace(msg, "$R", hero_link);
                    break;
            }
            if (msg != "")
            {
                GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
            }
        }
        m_smeltHeros[pos-1]->refresh();
        return HC_SUCCESS;
    }
    return HC_ERROR;
}

int CharTotalHeros::GoldenHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int golden_hero = 0, cnt = 0, star = 0;
    READ_INT_FROM_MOBJ(golden_hero,o,"golden_hero");
    READ_INT_FROM_MOBJ(cnt,o,"count");
    if (cnt > 20)
        cnt = 20;
    boost::shared_ptr<CharHeroData> g_hero = GetHero(golden_hero);
    if (!g_hero.get() || !g_hero->m_baseHero.get() || g_hero->m_baseHero->m_epic > 0)
    {
        ERR();
        return HC_ERROR;
    }
    star = g_hero->m_star;
    if (star < 1 || star >= iMaxHeroStar)
    {
        return HC_ERROR;
    }
    //炼金房等级决定可点金星级上限
    int c_star = cm->getGoldenMaxStar();
    if (c_star < star + 1)
    {
        return HC_ERROR_NEED_MORE_METALLURGY_LEVEL;
    }
    obj.push_back( Pair("silver_org", iGoldenHeroSilver[star-1]));
    obj.push_back( Pair("silver_cost", iGoldenHeroSilver[star-1] - cnt * iGoldenHeroCostSub));
    int tid = 0, up_star = 0;
    if (star < 4)
    {
        tid = GEM_ID_HERO_GOLD_4;
        up_star = 4;
    }
    else if (star == 4)
    {
        tid = GEM_ID_HERO_GOLD_5;
        up_star = 5;
    }
    json_spirit::Object hero_obj;
    g_hero->toObj(hero_obj,up_star);
    obj.push_back( Pair("hero_obj", hero_obj));
    json_spirit::Object tmp;
    Item item(ITEM_TYPE_GEM, tid, 1, 0);
    item.toObj(tmp);
    obj.push_back( Pair("gem_info", tmp));
    return HC_SUCCESS;
}

int CharTotalHeros::GoldenHero(json_spirit::Object& obj, json_spirit::mObject& o)
{
    boost::shared_ptr<char_metallurgy> cm = Singleton<cityMgr>::Instance().getCharMetallurgy(m_charData.m_id);
    if (!cm.get())
    {
        return HC_ERROR;
    }
    int golden_hero = 0, cnt = 0, star = 0;
    READ_INT_FROM_MOBJ(golden_hero,o,"golden_hero");
    boost::shared_ptr<CharHeroData> g_hero = GetHero(golden_hero);
    if (!g_hero.get() || !g_hero->m_baseHero.get() || g_hero->m_baseHero->m_epic > 0)
    {
        ERR();
        return HC_ERROR;
    }
    star = g_hero->m_star;
    if (star < 1 || star >= iMaxHeroStar)
    {
        return HC_ERROR;
    }
    //炼金房等级决定可点金星级上限
    int c_star = cm->getGoldenMaxStar();
    if (c_star < star + 1)
    {
        return HC_ERROR_NEED_MORE_METALLURGY_LEVEL;
    }
    int tid = 0, up_star = 0;
    if (star < 4)
    {
        tid = GEM_ID_HERO_GOLD_4;
        up_star = 4;
    }
    else if (star == 4)
    {
        tid = GEM_ID_HERO_GOLD_5;
        up_star = 5;
    }
    std::list<int> hero_list;
    json_spirit::mArray list;
    READ_ARRAY_FROM_MOBJ(list,o,"hero_list");
    json_spirit::mArray::iterator it = list.begin();
    while (it != list.end())
    {
        if ((*it).type() != json_spirit::obj_type)
        {
            ++it;
            continue;
        }
        json_spirit::mObject& tmp_obj = (*it).get_obj();
        int tmp_id = 0;
        READ_INT_FROM_MOBJ(tmp_id,tmp_obj,"id");
        boost::shared_ptr<CharHeroData> p_hero = GetHero(tmp_id);
        if (!p_hero.get() || !p_hero->m_baseHero.get() || p_hero->m_baseHero->m_epic > 0)
        {
            ERR();
            return HC_ERROR;
        }
        if (p_hero->isWork())
        {
            return HC_ERROR_DEFAULT_HERO;
        }
        if (p_hero->m_star != up_star - 1)
        {
            ERR();
            return HC_ERROR;
        }
        hero_list.push_back(tmp_id);
        ++cnt;
        ++it;
    }
    int silver_cost = iGoldenHeroSilver[star-1] - cnt * iGoldenHeroCostSub;
    if (m_charData.silver() < silver_cost)
        return HC_ERROR_NOT_ENOUGH_SILVER;
    if (m_charData.subGem(tid,1,gem_cost_hero_golden) < 0)
    {
        return HC_ERROR_NOT_ENOUGH_GEM;
    }
    m_charData.subSilver(silver_cost, silver_cost_hero_golden);
    for(std::list<int>::iterator it_h = hero_list.begin(); it_h != hero_list.end(); ++it_h)
    {
        if (Sub(*it_h) != 0)
        {
            ERR();
            INFO("Sub hid=" << *it_h <<endl);
        }
    }
    g_hero->updateStar(up_star);
    std::string msg = "";
    std::string hero_link = "";
    switch (up_star)
    {
        case 4:
            msg = strGoldenHeroStar4Msg;
            str_replace(msg, "$W", MakeCharNameLink(m_charData.m_name,m_charData.m_nick.get_string()));
            hero_link = MakeHeroLink(g_hero->m_baseHero->m_name, m_charData.m_id, g_hero->m_id, g_hero->m_star);
            addColor(hero_link,g_hero->m_star);
            str_replace(msg, "$R", hero_link);
            break;
        case 5:
            msg = strGoldenHeroStar5Msg;
            str_replace(msg, "$W", MakeCharNameLink(m_charData.m_name,m_charData.m_nick.get_string()));
            hero_link = MakeHeroLink(g_hero->m_baseHero->m_name, m_charData.m_id, g_hero->m_id, g_hero->m_star);
            addColor(hero_link,g_hero->m_star);
            str_replace(msg, "$R", hero_link);
            break;
    }
    if (msg != "")
    {
        GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
    }
    m_charData.m_tasks.updateTask(GOAL_GOLDEN_HERO, g_hero->m_star, 1);
    m_charData.m_tasks.updateTask(GOAL_DAILY_GOLDEN_HERO, g_hero->m_star, 1);
    //广播好友祝贺
    if (g_hero->m_star == 4)
    {
        Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_GOLDEN_QUALITY4_HERO, 0, 0);
    }
    else if (g_hero->m_star == 5)
    {
        Singleton<relationMgr>::Instance().postCongradulation(m_charData.m_id, CONGRATULATION_GOLDEN_QUALITY5_HERO, 0, 0);
    }
    return HC_SUCCESS;
}

int CharTotalHeros::UpEpicHeroInfo(json_spirit::Object& obj, json_spirit::mObject& o)
{
    int epic_hero = 0;
    READ_INT_FROM_MOBJ(epic_hero,o,"hid");
    boost::shared_ptr<baseHeroData> e_hero = Singleton<HeroMgr>::Instance().GetBaseHero(epic_hero);
    if (!e_hero.get())
    {
        ERR();
        return HC_ERROR;
    }
    json_spirit::Object hero_obj;
    e_hero->toEpicObj(hero_obj);
    obj.push_back( Pair("hero_obj", hero_obj));
    if (e_hero->m_next_hid > 0)
    {
        hero_obj.clear();
        boost::shared_ptr<baseHeroData> next = Singleton<HeroMgr>::Instance().GetBaseHero(e_hero->m_next_hid);
        if (next.get())
        {
            next->toEpicObj(hero_obj);
            obj.push_back( Pair("up_hero_obj", hero_obj));
        }

        baseHeroData* bh = e_hero.get();
        json_spirit::Object tmp;
        Item item_f(ITEM_TYPE_GEM, bh->m_epic_data.m_fragment_id, bh->m_epic_data.m_up_cost, 0);
        item_f.toObj(tmp);
        obj.push_back( Pair("fragment_info", tmp));

        tmp.clear();
        Item item_s(ITEM_TYPE_GEM, bh->m_epic_data.m_scroll_id, 1, 0);
        item_s.toObj(tmp);
        obj.push_back( Pair("scroll_info", tmp));
        obj.push_back( Pair("up_level", bh->m_epic_data.m_up_level));
        obj.push_back( Pair("up_vip", bh->m_epic_data.m_up_vip));
    }
    return HC_SUCCESS;
}

int CharTotalHeros::UpEpicHero(json_spirit::Object& obj, json_spirit::mObject& o)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"hid");
    int cur_id = GetHeroByType(id);
    boost::shared_ptr<CharHeroData> hero = GetHero(cur_id);
    if (!hero.get() || !hero->m_baseHero.get() || hero->m_baseHero->m_epic == 0)
    {
        ERR();
        return HC_ERROR;
    }
    if (hero->m_baseHero->m_next_hid == 0)
    {
        ERR();
        return HC_ERROR;
    }
    boost::shared_ptr<baseHeroData> next = Singleton<HeroMgr>::Instance().GetBaseHero(hero->m_baseHero->m_next_hid);
    if (!next.get())
    {
        ERR();
        return HC_ERROR;
    }
    if (m_charData.m_level < hero->m_baseHero->m_epic_data.m_up_level)
        return HC_ERROR_NEED_MORE_LEVEL;
    if (m_charData.m_vip < hero->m_baseHero->m_epic_data.m_up_vip)
        return HC_ERROR_NEED_MORE_VIP;
    //判断材料数量
    int gem_cnt = m_charData.m_bag.getGemCount(hero->m_baseHero->m_epic_data.m_fragment_id);
    int gem_common_cnt = m_charData.m_bag.getGemCount(GEM_ID_ELITE_HERO_COST_COMMON);
    if ((gem_cnt + gem_common_cnt) < hero->m_baseHero->m_epic_data.m_up_cost)
        return HC_ERROR_NOT_ENOUGH_GEM;
    //判断卷轴数量
    if (m_charData.m_bag.getGemCount(hero->m_baseHero->m_epic_data.m_scroll_id) < 1)
        return HC_ERROR_NOT_ENOUGH_GEM;
    //扣除材料
    if (hero->m_baseHero->m_epic_data.m_fragment_id > 0 && hero->m_baseHero->m_epic_data.m_up_cost > 0)
    {
        if (gem_cnt >= hero->m_baseHero->m_epic_data.m_up_cost)
        {
            m_charData.subGem(hero->m_baseHero->m_epic_data.m_fragment_id,hero->m_baseHero->m_epic_data.m_up_cost,gem_cost_hero_epic);
        }
        else
        {
            m_charData.subGem(hero->m_baseHero->m_epic_data.m_fragment_id,gem_cnt,gem_cost_hero_epic);
            int tmp = hero->m_baseHero->m_epic_data.m_up_cost - gem_cnt;
            m_charData.subGem(GEM_ID_ELITE_HERO_COST_COMMON,tmp,gem_cost_hero_epic);
        }
    }
    //扣除卷轴
    if (hero->m_baseHero->m_epic_data.m_scroll_id > 0)
    {
        m_charData.subGem(hero->m_baseHero->m_epic_data.m_scroll_id,1,gem_cost_hero_epic);
    }
    //替换英雄
    hero->m_hid = hero->m_baseHero->m_next_hid;
    hero->m_spic = next->m_spic;
    hero->m_race = next->m_race;
    hero->m_star = next->m_quality;
    if (next->m_epic)
    {
        hero->m_add[0] = next->m_epic_data.m_add[0];
        hero->m_add[1] = next->m_epic_data.m_add[1];
        hero->m_add[2] = next->m_epic_data.m_add[2];
        hero->m_add[3] = next->m_epic_data.m_add[3];
    }
    hero->m_quality = next->m_quality;
    hero->m_attack = next->m_base_attack;
    hero->m_defense = next->m_base_defense;
    hero->m_magic = next->m_base_magic;
    hero->m_hp = next->m_base_hp;
    hero->m_baseHero = next;
    hero->m_changed = true;
    hero->updateAttribute();
    //修改基础id
    InsertSaveDb("update char_heros set hid=" + LEX_CAST_STR(hero->m_hid) + " where id=" + LEX_CAST_STR(hero->m_id));
    obj.push_back( Pair("id", hero->m_id));
    boost::shared_ptr<CharHeroData> new_data = GetHero(hero->m_id);
    if (new_data.get())
    {
        //系统公告
        std::string notify_msg = strEpicHeroGet;
        str_replace(notify_msg, "$N", MakeCharNameLink(m_charData.m_name, m_charData.m_nick.get_string()));
        std::string hero_link = MakeHeroLink(next->m_name, m_charData.m_id, hero->m_id, new_data->m_star);
        addColor(hero_link,next->m_quality);
        str_replace(notify_msg, "$R", hero_link);
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
    }
    return HC_SUCCESS;
}

void CharTotalHeros::updateAttribute()
{
    //英雄属性更新
    std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.begin();
    while (it != m_heros.end())
    {
        if (it->second.get())
        {
            CharHeroData* hd = it->second.get();
            hd->updateAttribute();
        }
        ++it;
    }
}

int CharTotalHeros::Save()
{
    if (m_changed)
    {
        std::map<int, boost::shared_ptr<CharHeroData> >::iterator it = m_heros.begin();
        while (it != m_heros.end())
        {
            if (it->second.get())
            {
                it->second->Save();
            }
            ++it;
        }
        m_changed = false;
    }
    return 0;
}

int base_hero_pack::randomStar()
{
    boost::mt19937& gen = muduo::ThreadLocalSingleton<boost::mt19937>::instance();
    boost::random::discrete_distribution<> dist(m_gailvs);
    return dist(gen) + 1;
}

void base_hero_pack::toObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_id));
    obj.push_back( Pair("name", m_name));
    obj.push_back( Pair("quality", m_quality));
    obj.push_back( Pair("level", m_level));
    obj.push_back( Pair("vip", m_vip));
    obj.push_back( Pair("silver", m_silver_cost));
    obj.push_back( Pair("gold", m_gold_cost));
    json_spirit::Array list;
    for(int i = 0; i < m_gailvs.size(); ++i)
    {
        if (m_gailvs[i] > 0)
        {
            json_spirit::Object o;
            o.push_back( Pair("star", i+1));
            if (m_quality == 4 && i == 3)//大师包四星卡显示必出
            {
                o.push_back( Pair("per", 100));
            }
            else if(m_quality == 5 && i == 4)
            {
                o.push_back( Pair("per", 100));
            }
            else
            {
                o.push_back( Pair("per", m_gailvs[i] * 100 / m_total_gailv));
            }
            list.push_back(o);
        }
    }
    obj.push_back( Pair("list", list));
    return;
}

HeroMgr::HeroMgr()
{
    Query q(GetDb());
    //初始化基础英雄信息
    q.get_result("SELECT hid,inpack,race,spic,quality,name,memo,base_attack,base_defense,base_magic,base_hp,be_epic,next_hid FROM base_heros WHERE 1 order by hid");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int hid = q.getval();
        int inpack = q.getval();
        int race = q.getval();
        boost::shared_ptr<baseHeroData> p_herodata = GetBaseHero(hid);
        if (!p_herodata.get())
        {
            p_herodata.reset(new baseHeroData);
            m_base_heros[hid] = p_herodata;
            if (inpack)
            {
                m_pack_heros.push_back(hid);
                if (race >= 1 && race <= 4)
                {
                    m_race_heros[race].push_back(hid);
                }
            }
        }
        p_herodata->m_hid = hid;
        p_herodata->m_race = race;
        p_herodata->m_spic = q.getval();
        p_herodata->m_quality = q.getval();
        p_herodata->m_name = q.getstr();
        p_herodata->m_memo = q.getstr();
        p_herodata->m_base_attack = q.getval();
        p_herodata->m_base_defense = q.getval();
        p_herodata->m_base_magic = q.getval();
        p_herodata->m_base_hp = q.getval();
        p_herodata->m_epic = q.getval();
        p_herodata->m_next_hid = q.getval();
        p_herodata->loadMeterial();
        p_herodata->loadEpicData();
        if (p_herodata->m_epic && p_herodata->m_next_hid > 0)
        {
            m_epic_heros.push_back(hid);
        }
    }
    q.free_result();
    //初始化英雄包
    q.get_result("SELECT id,name,quality,level,vip,silver_cost,gold_cost,hero_cnt,race,per1,per2,per3,per4,per5 FROM base_hero_pack WHERE 1 order by id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int id = q.getval();
        boost::shared_ptr<base_hero_pack> p_heropack = GetBaseHeroPack(id);
        if (!p_heropack.get())
        {
            p_heropack.reset(new base_hero_pack);
            m_base_heropacks[id] = p_heropack;
        }
        p_heropack->m_id = id;
        p_heropack->m_name = q.getstr();
        p_heropack->m_quality = q.getval();
        p_heropack->m_level = q.getval();
        p_heropack->m_vip = q.getval();
        p_heropack->m_silver_cost = q.getval();
        p_heropack->m_gold_cost = q.getval();
        p_heropack->m_hero_cnt = q.getval();
        p_heropack->m_race_pack = q.getval();
        p_heropack->m_total_gailv = 0.0;
        for(int i = 0; i < iMaxHeroStar; ++i)
        {
            int gailv = q.getval();
            p_heropack->m_gailvs.push_back(gailv);
            p_heropack->m_total_gailv += gailv * 1.0;
        }
    }
    q.free_result();
}

//获得基础英雄信息
boost::shared_ptr<baseHeroData> HeroMgr::GetBaseHero(int hid)
{
    std::map<int, boost::shared_ptr<baseHeroData> >::iterator it = m_base_heros.find(hid);
    if (it != m_base_heros.end())
    {
        return it->second;
    }
    else
    {
        boost::shared_ptr<baseHeroData> p;
        p.reset();
        return p;
    }
}

//获得基础英雄包
boost::shared_ptr<base_hero_pack> HeroMgr::GetBaseHeroPack(int id)
{
    std::map<int, boost::shared_ptr<base_hero_pack> >::iterator it = m_base_heropacks.find(id);
    if (it != m_base_heropacks.end())
    {
        return it->second;
    }
    else
    {
        //ERR();
        //cout<<"!!!!hero_pack_type"<<id<<endl;
        boost::shared_ptr<base_hero_pack> p;
        p.reset();
        return p;
    }
}

int HeroMgr::OpenHeroPack(json_spirit::Object& obj, json_spirit::mObject& o, boost::shared_ptr<CharData> cdata)
{
    int id = 1, type = 1;
    READ_INT_FROM_MOBJ(id,o,"id");
    READ_INT_FROM_MOBJ(type,o,"type");
    boost::shared_ptr<base_hero_pack> p_heropack = GetBaseHeroPack(id);
    if (!p_heropack.get())
    {
        return HC_ERROR;
    }
    if ((cdata->m_heros.m_hero_max - cdata->m_heros.m_heros.size()) < p_heropack->m_hero_cnt)
        return HC_ERROR_HERO_FULL;
    if (p_heropack->m_level > 0 && p_heropack->m_vip > 0)
    {
        if (cdata->m_vip < p_heropack->m_vip && cdata->m_level < p_heropack->m_level)
        {
            return HC_ERROR_NEED_MORE_VIP;
        }
    }
    else
    {
        if (cdata->m_vip < p_heropack->m_vip)
        {
            return HC_ERROR_NEED_MORE_VIP;
        }
        if (cdata->m_level < p_heropack->m_level)
        {
            return HC_ERROR_NEED_MORE_LEVEL;
        }
    }
    bool action_buy = false;
    //扣除资源
    if (type == 1)//筹码购买
    {
        int cost_silver = p_heropack->m_silver_cost;
        if (cdata->subSilver(cost_silver, silver_cost_hero_pack) < 0)
            return HC_ERROR_NOT_ENOUGH_SILVER;
    }
    else if(type == 2)//金币购买
    {
        int cost_gold = p_heropack->m_gold_cost;
        //只能用金币购买的包判断活动
        if (p_heropack->m_silver_cost == 0)
        {
            //活动更新
            if (Singleton<actionMgr>::Instance().isHeroPackActionOpen(cdata.get()))
            {
                int state = cdata->queryExtraData(char_data_type_daily, char_data_daily_hero_pack_action);
                if (state == 0)
                {
                    cost_gold /= 2;
                    action_buy = true;
                }
            }
        }
        if (cdata->subGold(cost_gold, gold_cost_hero_pack, true) < 0)
            return HC_ERROR_NOT_ENOUGH_GOLD;
    }
    else
    {
        return HC_ERROR;
    }
    if (action_buy)
    {
        cdata->setExtraData(char_data_type_daily, char_data_daily_hero_pack_action, 1);
    }
    int star4_cnt = 0, star5_cnt = 0;
    json_spirit::Array list;
    //生成奖励随机英雄
    for(int i = 0; i < p_heropack->m_hero_cnt; ++i)
    {
        int star = 0;
        do
        {
            if (p_heropack->m_quality == 4 && star4_cnt== 0)//大师包四星卡必出
            {
                star = 4;
            }
            else if (p_heropack->m_quality == 5 && star5_cnt== 0)//王者包五星卡必出
            {
                star = 5;
            }
            else
            {
                star = p_heropack->randomStar();
            }
            if (star == 4)
                ++star4_cnt;
            if (star == 5)
                ++star5_cnt;
        }while((star == 4 && star4_cnt > 1) || (star == 5 && star5_cnt > 1));
        int hid = RandomHero(p_heropack->m_race_pack);
        int level = my_random(1,10);
        int id = cdata->m_heros.Add(hid,level,star);
        if (id > 0)
        {
            json_spirit::Object tmp;
            boost::shared_ptr<CharHeroData> p_hero = cdata->m_heros.GetHero(id);
            if (p_hero.get())
            {
                p_hero->toObj(tmp);
                list.push_back(tmp);
            }
            if (star == 5 && p_heropack->m_quality == 5)
            {
                std::string msg = strHeroPackMsg;
                std::string hero_link = "";
                str_replace(msg, "$W", MakeCharNameLink(cdata->m_name,cdata->m_nick.get_string()));
                hero_link = MakeHeroLink(p_hero->m_baseHero->m_name, cdata->m_id, p_hero->m_id, p_hero->m_star);
                addColor(hero_link,p_hero->m_star);
                str_replace(msg, "$R", hero_link);
                if (msg != "")
                {
                    GeneralDataMgr::getInstance()->broadCastSysMsg(msg, -1);
                }
            }
            //广播好友祝贺
            if (star == 4)
            {
                Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_PACK_QUALITY4_HERO, 0, 0);
            }
            else if (star == 5)
            {
                Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_PACK_QUALITY5_HERO, 0, 0);
            }
        }
    }
    obj.push_back( Pair("list", list) );
    boost::shared_ptr<base_hero_pack> pbhp = Singleton<HeroMgr>::Instance().GetBaseHeroPack(id);
    if (pbhp)
    {
        json_spirit::Object tmp;
        pbhp->toObj(tmp);
        obj.push_back( Pair("info", tmp) );
    }
    //成功之后主动发送
    obj.push_back( Pair("cmd", "openHeroPack") );
    obj.push_back( Pair("s", HC_SUCCESS) );
    cdata->sendObj(obj);
    //更新任务
    cdata->m_tasks.updateTask(GOAL_HERO_PACK, id, 1);
    if (type == 1)//筹码购买
    {
        cdata->m_tasks.updateTask(GOAL_DAILY_HERO_PACK_SILVER, 0, 1);
    }
    else if(type == 2)//金币购买
    {
        cdata->m_tasks.updateTask(GOAL_DAILY_HERO_PACK_GOLD, 0, 1);
    }
    Singleton<goalMgr>::Instance().updateTask(cdata->m_id, GOAL_TYPE_HERO_PACK, pbhp->m_quality);
    return HC_SUCCESS_NO_RET;
}

//随机英雄id
int HeroMgr::RandomHero(int race)
{
    if (race >= 1 && race <= 4)
    {
        int idx = my_random(0, m_race_heros[race].size() - 1);
        return m_race_heros[race][idx];
    }
    else
    {
        int idx = my_random(0, m_pack_heros.size() - 1);
        return m_pack_heros[idx];
    }
    return 0;
}

int HeroMgr::GetEpicHero(json_spirit::Object& obj, json_spirit::mObject& o, boost::shared_ptr<CharData> cdata)
{
    int id = 0;
    READ_INT_FROM_MOBJ(id,o,"hid");
    boost::shared_ptr<baseHeroData> bh = GetBaseHero(id);
    if (!bh.get() || bh->m_epic == 0 || bh->m_next_hid == 0)
    {
        ERR();
        return HC_ERROR;
    }
    if (cdata->m_heros.GetHeroByType(id))
    {
        ERR();
        return HC_ERROR;
    }
    if (cdata->m_heros.isFull())
        return HC_ERROR_HERO_FULL;
    if (cdata->m_level < bh->m_epic_data.m_get_level)
        return HC_ERROR_NEED_MORE_LEVEL;
    if (cdata->m_vip < bh->m_epic_data.m_get_vip)
        return HC_ERROR_NEED_MORE_VIP;
    int gem_cnt = cdata->m_bag.getGemCount(bh->m_epic_data.m_fragment_id);
    int gem_common_cnt = cdata->m_bag.getGemCount(GEM_ID_ELITE_HERO_COST_COMMON);
    if ((gem_cnt + gem_common_cnt) < bh->m_epic_data.m_get_cost)
        return HC_ERROR_NOT_ENOUGH_GEM;
    if (bh->m_epic_data.m_fragment_id > 0 && bh->m_epic_data.m_get_cost > 0)
    {
        if (gem_cnt >= bh->m_epic_data.m_get_cost)
        {
            cdata->subGem(bh->m_epic_data.m_fragment_id,bh->m_epic_data.m_get_cost,gem_cost_hero_epic);
        }
        else
        {
            cdata->subGem(bh->m_epic_data.m_fragment_id,gem_cnt,gem_cost_hero_epic);
            int tmp = bh->m_epic_data.m_get_cost - gem_cnt;
            cdata->subGem(GEM_ID_ELITE_HERO_COST_COMMON,tmp,gem_cost_hero_epic);
        }
    }
    //增加英雄
    int new_id = cdata->m_heros.Add(id,1,bh->m_quality);
    obj.push_back( Pair("id", new_id));
    boost::shared_ptr<CharHeroData> new_data = cdata->m_heros.GetHero(new_id);
    if (new_data.get())
    {
        //系统公告
        std::string notify_msg = strEpicHeroGet;
        str_replace(notify_msg, "$N", MakeCharNameLink(cdata->m_name, cdata->m_nick.get_string()));
        std::string hero_link = MakeHeroLink(bh->m_name, cdata->m_id, new_id, new_data->m_star);
        addColor(hero_link,bh->m_quality);
        str_replace(notify_msg, "$R", hero_link);
        if (notify_msg != "")
        {
            GeneralDataMgr::getInstance()->broadCastSysMsg(notify_msg, -1);
        }
    }
    Singleton<relationMgr>::Instance().postCongradulation(cdata->m_id, CONGRATULATION_EPIC_HERO, id, 0);
    return HC_SUCCESS;
}

