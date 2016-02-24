
#include "libao.h"
#include <mysql/mysql.h>
#include "Database.h"
#include "Query.h"
#include "data.h"
#include "spls_errcode.h"
#include "utils_all.h"
#include "singleton.h"
#include "combat.h"
#include "statistics.h"
#include "net.h"

extern Database& GetDb();
extern void InsertSaveDb(const std::string& sql);
extern int giveLoots(CharData* cdata, std::list<Item>& getItems, int mapid, int level, int type, Combat* pCombat, json_spirit::Object* robj, bool isAttacker, int statistics_type);
int getSessionChar(net::session_ptr& psession, boost::shared_ptr<CharData>& cdata);

extern void ItemToObj(Item* sitem, boost::shared_ptr<json_spirit::Object>& sgetobj);

const json_spirit::Object& baseLibao::getObj() const
{
    return m_obj;
}

void baseLibao::getObj(json_spirit::Object& obj)
{
    obj.push_back( Pair("id", m_libao_id) );
    obj.push_back( Pair("name", m_name) );
    obj.push_back( Pair("memo", m_memo) );
    obj.push_back( Pair("spic", m_spic) );
    obj.push_back( Pair("quality", m_quality) );
    return;
}

const json_spirit::Array& baseLibao::getArray() const
{
    return m_item_list;
}

void baseLibao::getArray(CharData& cdata, json_spirit::Array& list)
{
    for (std::list<Item>::iterator it_i = m_list.begin(); it_i != m_list.end(); ++it_i)
    {
        //cout << "updateObj libao_id=" << m_libao_id << ",item_type=" << it_i->type << ",item_id=" << it_i->id << endl;
        boost::shared_ptr<json_spirit::Object> p_obj;
        Item item = *it_i;
        switch (it_i->type)
        {
            case item_type_silver_level:
                item.type = item_type_silver;
                item.nums = cdata.m_level * item.nums;
                break;
            case item_type_treasure_level:
                item.type = item_type_treasure;
                item.nums = cdata.m_level * item.nums;
                break;
            case item_type_prestige_level:
                item.type = item_type_prestige;
                item.nums = cdata.m_level * item.nums;
                break;
        }
            
        ItemToObj(&item, p_obj);
        if (p_obj.get())
        {
            list.push_back(*(p_obj.get()));
        }
    }
}

void baseLibao::updateObj()
{
    m_obj.clear();
    m_obj.push_back( Pair("id", m_libao_id) );
    m_obj.push_back( Pair("name", m_name) );
    m_obj.push_back( Pair("memo", m_memo) );
    m_obj.push_back( Pair("spic", m_spic) );
    m_obj.push_back( Pair("quality", m_quality) );
    json_spirit::Array getlist;
    m_item_list.clear();

    need_slot_num = 0;
    for (std::list<Item>::iterator it_i = m_list.begin(); it_i != m_list.end(); ++it_i)
    {
        //cout << "updateObj libao_id=" << m_libao_id << ",item_type=" << it_i->type << ",item_id=" << it_i->id << endl;
        boost::shared_ptr<json_spirit::Object> p_obj;
        ItemToObj(&(*it_i), p_obj);
        if (p_obj.get())
        {
            getlist.push_back(*(p_obj.get()));
        }
        json_spirit::Object obj;
        it_i->toObj(obj);
        m_item_list.push_back(obj);

        if (it_i->type == item_type_equipment || it_i->type == item_type_baoshi)
        {
            ++need_slot_num;
        }
        else if(it_i->type == item_type_libao)
        {
            need_slot_num += it_i->nums;
        }
        else if(it_i->type == item_type_treasure)
        {
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(it_i->id);
            if (tr.get() && tr->currency == 0)
            {
                ++need_slot_num;
            }
        }
    }
    m_obj.push_back( Pair("get", getlist) );
}

libao::libao(int id, baseLibao& base)
:iItem(iItem_type_libao, base.m_libao_id, id, 1)
,m_base(base)
{
    m_id = id;
}

std::string libao::name() const
{
    return m_base.m_name;
}

std::string libao::memo() const
{
    return m_base.m_memo;
}

uint16_t libao::getSpic() const
{
    return m_base.m_spic;
}

int libao::getQuality() const
{
    return m_base.m_quality;
}

void libao::Save()
{
    if (m_changed)
    {
        int32_t c = getCount();    
        if (c > 0)
        {
            CharData* pc = getChar();
            InsertSaveDb("replace into char_libao (id,libao_id,cid,slot) values ("
                +LEX_CAST_STR(getId())+","
                +LEX_CAST_STR(m_base.m_libao_id)+","
                +LEX_CAST_STR(pc!=NULL ? pc->m_id : 0)+","
                +LEX_CAST_STR((int)(getSlot()))+")");
        }
        else
        {
            InsertSaveDb("delete from char_libao where id=" + LEX_CAST_STR(getId()));
        }
        m_changed = false;
    }
}

libao_mgr* libao_mgr::m_handle = NULL;

libao_mgr* libao_mgr::getInstance()
{
    if (NULL == m_handle)
    {
        m_handle = new libao_mgr();
        m_handle->load();
    }
    return m_handle;
}

baseLibao* libao_mgr::getBaselibao(int id)
{
    return m_base_libaos[id];
}

int libao_mgr::getLevelLibao(int level)
{
    if (m_levelLibaos.find(level) != m_levelLibaos.end())
    {
        return m_levelLibaos[level];
    }
    else
    {
        return 0;
    }
}

std::string libao_mgr::getlibaoMemo(int sid)
{
    std::string memo = "";
    baseLibao* p = NULL;
    p = libao_mgr::getInstance()->getBaselibao(sid);
    if (p == NULL)
    {
        return memo;
    }
    return p->m_memo;
}

void libao_mgr::getLevelLibaoList(CharData& cdata, json_spirit::Array& rlist)
{
    for (std::map<int,int>::iterator it = m_levelLibaos.begin(); it != m_levelLibaos.end(); ++it)
    {
        //已经领取
        if (cdata.m_newbie_reward[it->first])
        {
            continue;
        }
        json_spirit::Object obj;
        obj.push_back( Pair("id", it->first) );
        obj.push_back( Pair("level", it->first) );
        baseLibao* p = NULL;
        p = libao_mgr::getInstance()->getBaselibao(it->second);
        if (p != NULL)
        {
            obj.push_back( Pair("list", p->getArray()) );
        }
        //std::string memo = "";
        //memo = libao_mgr::getInstance()->getlibaoMemo(it->second);
        //obj.push_back( Pair("award", memo) );
        if (cdata.m_level >= it->first)
        {
            obj.push_back( Pair("enable", 1) );
        }
        else
        {
            obj.push_back( Pair("enable", 0) );
        }
        rlist.push_back(obj);
    }
}

int libao_mgr::getLevelLibaoState(CharData& cdata)
{
    bool finish = true;
    //更新新手冲锋号是否可以获得
    for (std::map<int,int>::iterator it = m_levelLibaos.begin(); it != m_levelLibaos.end(); ++it)
    {
        if (cdata.m_newbie_reward[it->first])
        {
            
        }
        else if (cdata.m_level >= it->first)
        {
            return 1;//可以领取
        }
        else
        {
            finish = false;
        }
    }
    if (finish)
        return 2;//全部领完
    return 0;//无可领取
}

void libao_mgr::load()
{
    Query q(GetDb());
    q.get_result("select id,type,level,attack,stronghold,name,memo,spic,quality from base_libao where 1");
    while (q.fetch_row())
    {
        int libao_id = q.getval();
        int libao_type = q.getval();
        baseLibao*p = new baseLibao;
        p->m_libao_id = libao_id;
        p->m_type = libao_type;
        p->m_level = q.getval();
        p->m_attack = q.getval();
        p->m_stronghold = q.getval();
        p->m_name = q.getstr();
        p->m_memo = q.getstr();
        p->m_spic = q.getval();
        p->m_quality = q.getval();
        p->need_slot_num = 0;
        //p->updateObj();
        m_base_libaos[p->m_libao_id] = p;
        //等级礼包
        if (p->m_type == 1)
            m_levelLibaos[p->m_level] = p->m_libao_id;
        else if(p->m_type == 2)
            m_chengzhang_Libaos.push_back(p->m_libao_id);
    }
    q.free_result();
    
    q.get_result("select l.libao_id,l.reward_type,l.reward_id,l.reward_num from base_libao_reward as l where 1 order by l.libao_id");
    while (q.fetch_row())
    {
        int libao_id = q.getval();
        baseLibao* p = getBaselibao(libao_id);
        if (p != NULL)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            p->m_list.push_back(it);            
        }
        //cout << "push_into libao_id=" << libao_id << " now_size=" << p->m_list.size() << endl;
    }
    q.free_result();
    for (std::map<int, baseLibao* >::iterator it = m_base_libaos.begin(); it != m_base_libaos.end(); ++it)
    {
        it->second->updateObj();
    }
    m_libao_id = 0;
    q.get_result("select max(id) from char_libao where 1");
    CHECK_DB_ERR(q);
    if (q.fetch_row())
    {
        m_libao_id = q.getval();
    }
    q.free_result();

#ifdef QQ_PLAT
    //黄钻等级礼包
    q.get_result("select level,itemType,itemId,counts from base_qq_level_libao where 1 order by level,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        baseLibao* lb = getQQLevelLibao(level);
        if (NULL == lb)
        {
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_qq_levelLibaos[level] = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_level = level;
            lb->m_name = "qq yellow level";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;            
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            lb->m_list.push_back(it);
        }
    }
    q.free_result();

    //黄钻新手礼包
    q.get_result("select itemType,itemId,counts from base_qq_newbie_libao where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        //int level = q.getval();
        baseLibao* lb = getQQNewbieLibao();
        if (NULL == lb)
        {
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_qq_newbie_libao = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_level = 1;
            lb->m_name = "qq yellow newbie";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;            
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            lb->m_list.push_back(it);
        }
    }
    q.free_result();

    //黄钻专属礼包
    q.get_result("select level,itemType,itemId,counts from base_qq_yellow_general where 1");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        baseLibao* lb = getQQSpecialLibao();
        if (NULL == lb)
        {
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_qq_special_libao = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_level = level;
            lb->m_name = "qq yellow special";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;            
        }
        else
        {
            if (level > lb->m_level)
            {
                lb->m_level = level;
            }
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            lb->m_list.push_back(it);
        }
    }
    q.free_result();

    //黄钻每日礼包
    q.get_result("select yellow_level,itemType,itemId,counts from base_qq_yellow_daily_libao where 1 order by yellow_level,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        //cout<<"qq yellow daily libao level "<<level<<endl;
        baseLibao* lb = NULL;
        if (level == 0)
        {
            lb = getQQYearDailyLibao();
            if (NULL == lb)
            {
                //cout<<"add qq year daily libao"<<endl;
                boost::shared_ptr<baseLibao> splb(new baseLibao);
                m_qq_year_daily_libao = splb;
                lb = splb.get();
                lb->m_libao_id = 0;
                lb->m_type = 0;
                lb->m_level = level;
                lb->m_name = "qq year yellow daily";
                lb->m_memo = "";
                lb->m_spic = 0;
                lb->m_quality = 0;
                lb->need_slot_num = 0;            
            }
        }
        else
        {
            lb = getQQDailyLibao(level);
            if (NULL == lb)
            {
                //cout<<"add qq daily libao"<<endl;
                boost::shared_ptr<baseLibao> splb(new baseLibao);
                m_qq_daily_libao[level] = splb;
                lb = splb.get();
                lb->m_libao_id = 0;
                lb->m_type = 0;
                lb->m_level = level;
                lb->m_name = "qq yellow daily";
                lb->m_memo = "";
                lb->m_spic = 0;
                lb->m_quality = 0;
                lb->need_slot_num = 0;            
            }
        }
        
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            lb->m_list.push_back(it);
            //cout<<"add qq year daily libao, add item "<<it.type<<","<<it.id<<","<<it.nums<<endl;
        }
    }
    q.free_result();

    if (m_qq_newbie_libao.get())
    {
        m_qq_newbie_libao->updateObj();
    }
    if (m_qq_special_libao.get())
    {
        m_qq_special_libao->updateObj();
    }
    if (m_qq_year_daily_libao.get())
    {
        m_qq_year_daily_libao->updateObj();
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_levelLibaos.begin(); it != m_qq_levelLibaos.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
        }
    }
    m_qq_daily_list.clear();
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_daily_libao.begin(); it != m_qq_daily_libao.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
            //cout<<"update obj ->"<<it->second->m_name<<",level:"<<it->second->m_level<<endl;
            //cout<<it->second->m_list.size()<<","<<it->second->m_item_list.size()<<endl;
            json_spirit::Object obj;
            obj.push_back( Pair("level", it->first) );
            obj.push_back( Pair("list", it->second->getArray()) );
            m_qq_daily_list.push_back(obj);
        }
    }
#else
    //Vip专属礼包
    baseLibao* vip_spe = new baseLibao;
    m_vip_special_libao.reset(vip_spe);
    
    vip_spe->m_libao_id = 0;
    vip_spe->m_type = 0;
    vip_spe->m_level = 30;
    vip_spe->m_name = "vip special";
    vip_spe->m_memo = "";
    vip_spe->m_spic = 0;
    vip_spe->m_quality = 0;
    vip_spe->need_slot_num = 0;
    Item vip_general;
    vip_general.type = item_type_general;
    vip_general.id = 999;
    vip_general.spic = 999;
    vip_general.nums = 1;
    vip_spe->m_list.push_back(vip_general);

    //VIP每日礼包
    q.get_result("select vip,itemType,itemId,counts from base_vip_daily_libao where 1 order by vip,id");
    CHECK_DB_ERR(q);
    while (q.fetch_row())
    {
        int level = q.getval();
        //cout<<"qq yellow daily libao level "<<level<<endl;
        baseLibao* lb = NULL;
        lb = getVipDailyLibao(level);
        if (NULL == lb)
        {
            //cout<<"add qq year daily libao"<<endl;
            boost::shared_ptr<baseLibao> splb(new baseLibao);
            m_vip_daily_libao[level] = splb;
            lb = splb.get();
            lb->m_libao_id = 0;
            lb->m_type = 0;
            lb->m_level = level;
            lb->m_name = "vip daily";
            lb->m_memo = "";
            lb->m_spic = 0;
            lb->m_quality = 0;
            lb->need_slot_num = 0;            
        }
        if (lb)
        {
            Item it;
            it.type = q.getval();
            it.id = q.getval();
            it.nums = q.getval();
            lb->m_list.push_back(it);
            //cout<<"add qq year daily libao, add item "<<it.type<<","<<it.id<<","<<it.nums<<endl;
        }
    }
    q.free_result();

    //VIP周福利
    m_vip_week_libao_org_gold = 0;
    m_vip_week_libao_gold = 0;
    q.get_result("select vip,type,count,discount from base_vip_week_benefit where 1 order by vip,id");
    while (q.fetch_row())
    {
        int vip = q.getval();
        if (vip == 0)
        {
            Item item;
            item.type = item_type_treasure;
            item.id = q.getval();
            item.nums = q.getval();

            if (item.nums < 0 || item.nums > 10000)
            {
                ERR();
                continue;
            }
            int discount = q.getval();
            if (discount <= 0)
            {
                ERR();
                continue;
            }

            baseLibao* lb = NULL;
            lb = getVipWeekLibao();
            if (NULL == lb)
            {
                //cout<<"add qq year daily libao"<<endl;
                boost::shared_ptr<baseLibao> splb(new baseLibao);
                m_vip_week_libao = splb;
                lb = splb.get();
                lb->m_libao_id = 0;
                lb->m_type = 0;
                lb->m_level = 0;
                lb->m_name = "vip week";
                lb->m_memo = "";
                lb->m_spic = 0;
                lb->m_quality = 0;
                lb->need_slot_num = 0;
            }
            if (lb)
            {
                lb->m_list.push_back(item);
            }

            boost::shared_ptr<baseTreasure> bt = GeneralDataMgr::getInstance()->GetBaseTreasure(item.id);
            if (bt.get())
            {
                if (bt->gold_to_buy > 0)
                {
                    m_vip_week_libao_org_gold += (bt->gold_to_buy * item.nums);
                    if (discount < 100)
                    {
                        m_vip_week_libao_gold += (bt->gold_to_buy * item.nums * discount / 100);
                    }
                    else
                    {
                        m_vip_week_libao_gold += (bt->gold_to_buy * item.nums);
                    }
                }
            }
        }
    }
    q.free_result();

    if (m_vip_special_libao.get())
    {
        m_vip_special_libao->updateObj();
    }

    if (m_vip_week_libao.get())
    {
        m_vip_week_libao->updateObj();
    }

    m_vip_daily_list.clear();
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_vip_daily_libao.begin(); it != m_vip_daily_libao.end(); ++it)
    {
        if (it->second.get())
        {
            it->second->updateObj();
            //cout<<"update obj ->"<<it->second->m_name<<",level:"<<it->second->m_level<<endl;
            //cout<<it->second->m_list.size()<<","<<it->second->m_item_list.size()<<endl;
            json_spirit::Object obj;
            obj.push_back( Pair("level", it->first) );
            obj.push_back( Pair("list", it->second->getArray()) );
            m_vip_daily_list.push_back(obj);
        }
    }
    
#endif
}

//增加礼包
int libao_mgr::addLibao(CharData* pc, int libao_id)
{
    if (!pc)
    {
        return HC_ERROR;
    }
    if (pc->m_bag.isFull())
    {
        return HC_ERROR_BAG_FULL;
    }
    boost::shared_ptr<iItem> bs;
    baseLibao* pl = getBaselibao(libao_id);
    if (!pl)
    {
         return HC_ERROR;
    }
    int id = newLibaoId();
    libao* pb = new libao(id, *pl);
    bs.reset(pb);
    if (!bs.get())
    {
        return HC_ERROR;
    }
    pc->m_bag.addItem(bs);
    pb->Save();
    json_spirit::Object obj;
    json_spirit::Object item;
    item.push_back( Pair("type", item_type_libao) );
    item.push_back( Pair("spic", pl->m_spic) );
    obj.push_back( Pair("item", item) );
    obj.push_back( Pair("cmd", "notify") );
    obj.push_back( Pair("s", 200) );
    obj.push_back( Pair("type", notify_msg_new_get) );
    pc->sendObj(obj);
    return HC_SUCCESS;
}

#ifdef QQ_PLAT
baseLibao* libao_mgr::getQQLevelLibao(int level)
{
    if (m_qq_levelLibaos.find(level) != m_qq_levelLibaos.end())
    {
        return m_qq_levelLibaos[level].get();
    }
    else
    {
        return NULL;
    }
}

//是否已经领取全部qq成长礼包
bool libao_mgr::allGetQQLevelLibao(CharData* pc)
{
    if (!pc)
    {
        return false;
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_levelLibaos.begin(); it != m_qq_levelLibaos.end(); ++it)
    {
        if (pc->queryExtraData(char_data_type_normal, char_data_qq_yellow_level_libao + it->first))
        {
        }
        else
        {
            return false;
        }
    }
    return true;
}

baseLibao* libao_mgr::getQQDailyLibao(int yellow_level)
{
    if (m_qq_daily_libao.find(yellow_level) != m_qq_daily_libao.end())
    {
        return m_qq_daily_libao[yellow_level].get();
    }
    else
    {
        return NULL;
    }
}

void libao_mgr::getQQDailyList(CharData& cdata, json_spirit::Array& list, json_spirit::Array& list2)
{
    if (m_qq_year_daily_libao.get())
    {
        m_qq_year_daily_libao->getArray(cdata, list2);
    }
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_daily_libao.begin(); it != m_qq_daily_libao.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            obj.push_back( Pair("level", it->first) );
            json_spirit::Array list1;
            it->second->getArray(cdata, list1);
            obj.push_back( Pair("list", list1) );
            list.push_back(list1);
        }
    }
}

baseLibao* libao_mgr::getQQYearDailyLibao()
{
    return m_qq_year_daily_libao.get();
}

baseLibao* libao_mgr::getQQNewbieLibao()
{
    return m_qq_newbie_libao.get();
}

baseLibao* libao_mgr::getQQSpecialLibao()
{
    return m_qq_special_libao.get();
}

void libao_mgr::getCharQQLevelLibao(CharData& cdata, json_spirit::Array& list)
{
    for (std::map<int, boost::shared_ptr<baseLibao> >::iterator it = m_qq_levelLibaos.begin(); it != m_qq_levelLibaos.end(); ++it)
    {
        if (it->second.get())
        {
            json_spirit::Object obj;
            if (cdata.m_qq_yellow_level > 0 && cdata.m_level >= it->first)
            {
                int get = cdata.queryExtraData(char_data_type_normal, it->first + char_data_qq_yellow_level_libao);
                if (get == 0)
                {
                    obj.push_back( Pair("canGet", 1) );
                }
                else
                {
                    continue;
                }
            }
            obj.push_back( Pair("level", it->first) );
            obj.push_back( Pair("list", it->second->getArray()) );

            list.push_back(obj);
        }
    }
}
#else

baseLibao* libao_mgr::getVipDailyLibao(int vip)
{
    if (m_vip_daily_libao.find(vip) != m_vip_daily_libao.end())
    {
        return m_vip_daily_libao[vip].get();
    }
    else
    {
        return NULL;
    }
}

baseLibao* libao_mgr::getVipWeekLibao()
{
    return m_vip_week_libao.get();
}

baseLibao* libao_mgr::getVipSpecialLibao()
{
    return m_vip_special_libao.get();
}

#endif

int libao_mgr::getChengzhangLibao(int pos)
{
    if (pos > 0 && pos <= m_chengzhang_Libaos.size())
    {
        return m_chengzhang_Libaos[pos-1];
    }
    else
    {
        return 0;
    }
}

int libao_mgr::isChengzhangLibao(int id)
{
    for (int i = 0; i <= m_chengzhang_Libaos.size(); ++i)
    {
        if (m_chengzhang_Libaos[i] == id)
        {
            return i+1;
        }
    }
    return 0;
}

int libao_mgr::getChengzhangState(CharData& cdata)
{
    while (cdata.m_chengzhang_reward.size() < m_chengzhang_Libaos.size())
        cdata.m_chengzhang_reward.push_back(0);
    for (int i = 0; i <= m_chengzhang_Libaos.size(); ++i)
    {
        if (cdata.m_chengzhang_reward[i] == 2)
        {
            continue;
        }
        else if (cdata.m_chengzhang_reward[i] == 1)
        {
            int libao_id = libao_mgr::getInstance()->getChengzhangLibao(i+1);
            if (cdata.m_bag.getChengzhangLibaoSlot(libao_id) == 0)
                continue;
            return i+1;
        }
        else if (cdata.m_chengzhang_reward[i] == 0)
        {
            int libao_id = libao_mgr::getInstance()->getChengzhangLibao(i+1);
            if (cdata.m_bag.getChengzhangLibaoSlot(libao_id) == 0)
                continue;
            baseLibao* p = NULL;
            p = libao_mgr::getInstance()->getBaselibao(libao_id);
            if (p == NULL)
            {
                continue;
            }
            if (cdata.m_level >= p->m_level && cdata.getAttack() >= p->m_attack && cdata.m_currentStronghold >= p->m_stronghold)
            {
                cdata.m_chengzhang_reward[i] = 1;
                InsertSaveDb("replace into char_chengzhang_event (cid,pos,state) values ("
                        + LEX_CAST_STR(cdata.m_id) + ","+ LEX_CAST_STR(i+1) + "," + LEX_CAST_STR(cdata.m_chengzhang_reward[i]) + ")");
                return i+1;
            }
        }
    }
    return 0;
}

//打开礼包
int CharData::openLibao(int slot, json_spirit::Object& robj, bool real_get)
{
    boost::shared_ptr<iItem> itm = m_bag.getItem(slot);
    if (!itm.get())
    {
        return HC_ERROR;
    }
    if (itm->getType() != iItem_type_libao)
    {
        return HC_ERROR;
    }
    libao* pl = dynamic_cast<libao*>(itm.get());
    //成长礼包特殊处理，打开界面
    int chengzhang_pos = libao_mgr::getInstance()->isChengzhangLibao(pl->m_base.m_libao_id);
    if (chengzhang_pos > 0 && !real_get)
    {
        robj.push_back( Pair("ischengzhang", 1) );
        return getChengzhangLibaoInfo(chengzhang_pos,robj);
    }
    int need_slot_num = pl->m_base.need_slot_num;
    std::list<Item>::iterator it = pl->m_base.m_list.begin();
    while (it != pl->m_base.m_list.end())
    {
        if (it->type == item_type_baoshi)
        {
            if (it->fac == 0)
                it->fac = 1;
            newBaoshi* p = m_bag.getBaoshiCanMerge(it->id, it->fac, 1);
            if (p)
            {
                --need_slot_num;
            }
        }
        else if (it->type == item_type_treasure)
        {
            boost::shared_ptr<baseTreasure> tr = GeneralDataMgr::getInstance()->GetBaseTreasure(it->id);
            if (tr.get() && tr->currency == 0)
            {
                int32_t max_c = tr->max_size;
                if (max_c > 1 || 0 == max_c)
                {
                    for (size_t i = 0; i < m_bag.m_size && it->nums > 0; ++i)
                    {
                        iItem* p = m_bag.m_bagslot[i].get();
                        if (p && p->getType() == iItem_type_gem && p->getSubtype() == it->id)
                        {
                            int32_t c = p->getCount();
                            if (0 == max_c || (c + it->nums) <= max_c)
                            {
                                --need_slot_num;
                                break;
                            }
                        }
                    }
                }
            }
        }
        ++it;
    }
    if (need_slot_num > 0 && (m_bag.size()-m_bag.getUsed()) < need_slot_num)
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }

    //打开礼包任务
    updateTask(task_open_libao, pl->m_base.m_libao_id);

    std::list<Item> items = pl->m_base.m_list;
    giveLoots(this, items, 0, m_level, 0, NULL, &robj, true, give_libao_loot);
    m_bag.removeItem(slot);
    itm->Clear();
    itm->Save();
    robj.push_back( Pair("refresh", 1) );
    robj.push_back( Pair("ischengzhang", 0) );
    return HC_SUCCESS;
}

//直接奖励礼包
int CharData::rewardLibao(int libao_id, json_spirit::Object& robj)
{
    baseLibao* p = NULL;
    p = libao_mgr::getInstance()->getBaselibao(libao_id);
    if (p == NULL)
    {
        return HC_ERROR;
    }
    if ((m_bag.size()-m_bag.getUsed()) < p->need_slot_num)
    {
        return HC_ERROR_NOT_ENOUGH_BAG_SIZE;
    }
    //给东西
    std::list<Item> items = p->m_list;
    giveLoots(this, items, 0, m_level, 0, NULL, &robj, true, give_libao_loot);
    return HC_SUCCESS;
}

int CharData::getLibaoInfo(int libao_id, json_spirit::Object& robj)
{
    baseLibao* p = NULL;
    p = libao_mgr::getInstance()->getBaselibao(libao_id);
    if (p == NULL)
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("libaoVO", p->getObj()) );
    return HC_SUCCESS;
}

int CharData::getChengzhangLibaoInfo(int pos, json_spirit::Object& robj)
{
    int libao_id = libao_mgr::getInstance()->getChengzhangLibao(pos);
    baseLibao* p = NULL;
    p = libao_mgr::getInstance()->getBaselibao(libao_id);
    if (p == NULL)
    {
        return HC_ERROR;
    }
    robj.push_back( Pair("state", m_chengzhang_reward[pos-1]) );
    p->getObj(robj);
    robj.push_back( Pair("pos", pos) );
    robj.push_back( Pair("list", p->getArray()) );
    return HC_SUCCESS;
}

#ifdef QQ_PLAT
//查询黄钻界面：cmd:getYellowEvent
int ProcessQueryQQYellowEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    robj.push_back( Pair("generalGet", cdata->queryExtraData(char_data_type_normal, char_data_qq_yellow_special) ) );
    robj.push_back( Pair("newbieGet", cdata->queryExtraData(char_data_type_normal, char_data_qq_yellow_newbie) ) );
    robj.push_back( Pair("levelGet", libao_mgr::getInstance()->allGetQQLevelLibao(cdata.get()) ? 1 : 0));
    return ret;
}

//查询黄钻新手礼包 cmd：queryQQnewbieLibao
int ProcessQueryQQNewbieLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    if (cdata->m_qq_yellow_level > 0)
    {
        int get = cdata->queryExtraData(char_data_type_normal, char_data_qq_yellow_newbie);
        robj.push_back( Pair("get", get));
    }
    baseLibao* lb = libao_mgr::getInstance()->getQQNewbieLibao();
    if (lb)
    {
        robj.push_back( Pair("list", lb->getArray()) );
    }
    return ret;
}

//查询黄钻每日礼包 cmd：queryQQDailyLibao
int ProcessQueryQQDailyLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    robj.push_back( Pair("get", cdata->queryExtraData(char_data_type_daily, char_data_daily_qq_yellow_libao)));
    robj.push_back( Pair("yearGet", cdata->queryExtraData(char_data_type_daily, char_data_daily_qq_year_yellow_libao)));

    //json_spirit::Array list1, list2;
    //libao_mgr::getInstance()->getQQDailyList(*cdata.get(), list1, list2);
    
    robj.push_back( Pair("list", libao_mgr::getInstance()->getQQDailyList()) );
    baseLibao* lb = libao_mgr::getInstance()->getQQYearDailyLibao();
    if (lb)
    {
        robj.push_back( Pair("list2", lb->getArray()) );
    }
    return ret;
}

//查询黄钻成长礼包 cmd：queryQQLevelLibao
int ProcessQueryQQLevelLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip_is_year_yellow", cdata->m_qq_yellow_year) );
    robj.push_back( Pair("vip_yellow_level", cdata->m_qq_yellow_level) );

    json_spirit::Array list;
    libao_mgr::getInstance()->getCharQQLevelLibao(*(cdata.get()), list);
    robj.push_back( Pair("list", list) );
    return ret;
}

#else

//查询VIP每日礼包 cmd：queryVipDailyLibao
int ProcessQueryVipDailyLibao(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip", cdata->m_vip) );

    robj.push_back( Pair("get", cdata->queryExtraData(char_data_type_daily, char_data_daily_vip_libao)));
    robj.push_back( Pair("weekGet", cdata->queryExtraData(char_data_type_week, char_data_week_vip_libao)));

    //json_spirit::Array list1, list2;
    //libao_mgr::getInstance()->getQQDailyList(*cdata.get(), list1, list2);
    
    robj.push_back( Pair("list", libao_mgr::getInstance()->getVipDailyList()) );
    baseLibao* lb = libao_mgr::getInstance()->getVipWeekLibao();
    if (lb)
    {
        robj.push_back( Pair("list2", lb->getArray()) );
        robj.push_back( Pair("orgGold", libao_mgr::getInstance()->vipWeekOrgGold()) );
        robj.push_back( Pair("gold", libao_mgr::getInstance()->vipWeekGold()) );
    }
    return ret;
}

#endif

//查询VIP特权界面：cmd:getVipBenefit
int ProcessQueryVipEvent(net::session_ptr& psession, json_spirit::mObject& o, json_spirit::Object& robj)
{
    boost::shared_ptr<CharData> cdata;
    int ret = getSessionChar(psession, cdata);
    if (ret != HC_SUCCESS || !cdata.get())
    {
        return ret;
    }
    robj.push_back( Pair("vip", cdata->m_vip) );

    int first_view = cdata->queryExtraData(char_data_type_daily, char_data_daily_view_vip_benefit);
    if (0 == first_view)
    {
        //每日第一次闪亮
        cdata->setExtraData(char_data_type_daily, char_data_daily_view_vip_benefit, 1);
    }

    int vip8_general_state = cdata->queryExtraData(char_data_type_normal, char_data_vip8_general);
    int vip10_general_state = cdata->queryExtraData(char_data_type_normal, char_data_vip10_general);
    
    robj.push_back( Pair("generalGet2", vip8_general_state ) );
    robj.push_back( Pair("generalGet3", vip10_general_state ) );
#ifdef QQ_PLAT
    int vip_general_state = cdata->queryExtraData(char_data_type_normal, char_data_qq_yellow_special);
#else
    int vip_general_state = cdata->queryExtraData(char_data_type_normal, char_data_vip_special_libao);
#endif
    robj.push_back( Pair("generalGet", vip_general_state ) );

    //vipGet:1      VIP礼包已经全部领取
    int state = 2;
    std::map<int,CharVIPPresent>::iterator it = cdata->m_vip_present.begin();
    while (it != cdata->m_vip_present.end())
    {
        CharVIPPresent cvp = it->second;
        if (cvp.present.get())
        {
            if (cvp.state == 1)
            {
                state = 1;
                break;
            }
            else if (state == 2)
            {
                state = 0;
            }
            break;
        }
        ++it;
    }
    robj.push_back( Pair("vipGet", state == 2 ? 1 : 0) );

    int cur = 3;
    if (vip_general_state == 0 && cdata->m_vip >= 5 && cdata->m_level >= 30)
    {
        cur = 1;
    }
    else if (vip8_general_state == 0 && cdata->m_vip >= 8 && cdata->m_level >= 40)
    {
        cur = 4;
    }
    else if (vip10_general_state == 0 && cdata->m_vip >= 10 && cdata->m_level >= 70)
    {
        cur = 5;
    }
    else if (1 == state)
    {
        cur = 2;
    }
#ifndef QQ_PLAT
    else if (cdata->m_vip > 0)
    {
        int vip_daily_get = cdata->queryExtraData(char_data_type_daily, char_data_daily_vip_libao);
        int vip_week_get = cdata->queryExtraData(char_data_type_week, char_data_week_vip_libao);
        if (vip_daily_get == 0 || vip_week_get == 0)
        {
            cur = 3;
        }
        else
        {
            cur = 2;
        }
    }
    else
    {
        cur = 2;
    }
#else
    else
    {
        cur = 2;
    }
#endif
    
    //cur:1武将,2VIP礼包,3VIP每日礼包,4专属橙将，5专属红将
    robj.push_back( Pair("cur", cur) );

    if (first_view == 0)
    {
        int state = cdata->getVipState();
        if (0 == state)
        {
            cdata->notifyEventState(top_level_event_vip_present, 0, 0);
        }
    }
    return ret;
}

